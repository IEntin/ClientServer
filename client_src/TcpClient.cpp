/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <iostream>

namespace tcp {

CloseSocket::CloseSocket(boost::asio::ip::tcp::socket& socket) : _socket(socket) {}

CloseSocket::~CloseSocket() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
}

bool processTask(boost::asio::ip::tcp::socket& socket,
		 const Batch& payload,
		 const TcpClientOptions& options,
		 std::ostream* dataStream) {
  // keep vector capacity
  static Batch modified;
  static const size_t bufferSize = MemoryPool::getInitialBufferSize();
  if (options._prepareOnce) {
    static bool done = utility::preparePackage(payload, modified, bufferSize, options._diagnostics);
    if (!done) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
  }
  else if (!utility::preparePackage(payload, modified, bufferSize, options._diagnostics))
    return false;
  for (const auto& chunk : modified) {
    boost::system::error_code ec;
    size_t result[[maybe_unused]] = boost::asio::write(socket, boost::asio::buffer(chunk), ec);
    if (ec) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ':' << ec.what() << std::endl;
      return false;
    }
    char header[HEADER_SIZE + 1] = {};
    memset(header, 0, HEADER_SIZE);
    boost::asio::read(socket, boost::asio::buffer(header, HEADER_SIZE), ec);
    auto [uncomprSize, comprSize, compressor, diagnostics, done] =
      decodeHeader(std::string_view(header, HEADER_SIZE), !ec);
    if (!done)
      return false;
    std::ostream* pstream = dataStream ? dataStream : options._dataStream;
    if (!readReply(socket, uncomprSize, comprSize, compressor == COMPRESSORS::LZ4, pstream))
      return false;
  }
  return true;
}

bool run(const Batch& payload, const TcpClientOptions& options, std::ostream* dataStream) {
  unsigned numberTasks = 0;
  try {
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::socket socket(ioContext);
    CloseSocket closeSocket(socket);
    boost::asio::ip::tcp::resolver resolver(ioContext);
    boost::system::error_code ec;
    boost::asio::connect(socket, resolver.resolve(options._serverHost, options._tcpPort, ec));
    if (ec) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ':' << ec.what() << std::endl;
      return false;
    }
    do {
      Chronometer chronometer(options._timing, __FILE__, __LINE__, __func__, options._instrStream);
      if (!processTask(socket, payload, options, dataStream))
	return false;
      // limit output file size
      if (++numberTasks == options._maxNumberTasks)
	break;
    } while (options._runLoop);
  }
  catch (const std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":exception:" << e.what() << std::endl;
    return false;
  }
  return true;
}

bool readReply(boost::asio::ip::tcp::socket& socket,
	       size_t uncomprSize,
	       size_t comprSize,
	       bool bcompressed,
	       std::ostream* pstream) {
  std::vector<char>& buffer = MemoryPool::getSecondaryBuffer(comprSize);
  boost::system::error_code ec;
  size_t transferred[[maybe_unused]] = boost::asio::read(socket, boost::asio::buffer(buffer, comprSize), ec);
  if (ec) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return false;
  }
  std::string_view received(buffer.data(), comprSize);
  std::ostream& stream = pstream ? *pstream : std::cout;
  if (bcompressed) {
    std::string_view dstView = Compression::uncompress(received, uncomprSize);
    if (dstView.empty()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
      return false;
    }
    stream << dstView; 
  }
  else
    stream << received;
  return true;
}

} // end of namespace tcp
