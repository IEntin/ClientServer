/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include <iostream>

namespace tcp {

CloseSocket::CloseSocket(boost::asio::ip::tcp::socket& socket) : _socket(socket) {}

CloseSocket::~CloseSocket() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
}

TcpClient::TcpClient(const TcpClientOptions& options) :
  Client(options), _ioContext(1), _socket(_ioContext), _options(options) {}

bool TcpClient::processTask(const Batch& payload) {
  static const size_t bufferSize = MemoryPool::getInitialBufferSize();
  if (_options._buildTaskOnce) {
    static bool done = buildTask(payload, bufferSize);
    if (!done) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
  }
  else if (!buildTask(payload, bufferSize))
    return false;
  for (const auto& subtask : _task) {
    boost::system::error_code ec;
    size_t result[[maybe_unused]] = boost::asio::write(_socket, boost::asio::buffer(subtask), ec);
    if (ec) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ':' << ec.what() << std::endl;
      return false;
    }
    char header[HEADER_SIZE + 1] = {};
    memset(header, 0, HEADER_SIZE);
    boost::asio::read(_socket, boost::asio::buffer(header, HEADER_SIZE), ec);
    auto [uncomprSize, comprSize, compressor, diagnostics, done] =
      decodeHeader(std::string_view(header, HEADER_SIZE), !ec);
    if (!done)
      return false;
    if (!readReply(uncomprSize, comprSize, compressor == COMPRESSORS::LZ4))
      return false;
  }
  return true;
}

// For the test payload is unchanged in a loop.

bool TcpClient::run(const Batch& payload) {
  unsigned numberTasks = 0;
  try {
    CloseSocket closeSocket(_socket);
    boost::asio::ip::tcp::resolver resolver(_ioContext);
    boost::system::error_code ec;
    auto endpoint = boost::asio::connect(_socket,
					 resolver.resolve(_options._serverHost, _options._tcpPort, ec));
    if (!ec)
      _socket.set_option(boost::asio::ip::tcp::acceptor::reuse_address(false), ec);
    if (!ec)
      _socket.set_option(boost::asio::socket_base::linger(false, 0), ec);
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " endpoint: " << endpoint << std::endl;
    if (ec) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ':' << ec.what() << std::endl;
      return false;
    }
    do {
      Chronometer chronometer(_options._timing, __FILE__, __LINE__, __func__, _options._instrStream);
      if (!processTask(payload))
	return false;
      // limit output file size
      if (++numberTasks == _options._maxNumberTasks)
	break;
    } while (_options._runLoop);
  }
  catch (const std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":exception:" << e.what() << std::endl;
    return false;
  }
  return true;
}

bool TcpClient::readReply(size_t uncomprSize, size_t comprSize, bool bcompressed) {
  std::vector<char>& buffer = MemoryPool::getSecondaryBuffer(comprSize);
  boost::system::error_code ec;
  size_t transferred[[maybe_unused]] = boost::asio::read(_socket, boost::asio::buffer(buffer, comprSize), ec);
  if (ec) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return false;
  }
  std::string_view received(buffer.data(), comprSize);
  std::ostream* pstream = _options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
						       << " received compressed" << std::endl;
    std::string_view dstView = Compression::uncompress(received, uncomprSize);
    if (dstView.empty()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
      return false;
    }
    stream << dstView; 
  }
  else {
    static auto& printOnce[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
						       << " received not compressed" << std::endl;
    stream << received;
  }
  return true;
}

} // end of namespace tcp
