/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TcpClient.h"
#include "ClientOptions.h"
#include "Header.h"
#include <iostream>

namespace tcp {

CloseSocket::CloseSocket(boost::asio::ip::tcp::socket& socket) : _socket(socket) {}

CloseSocket::~CloseSocket() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
}

TcpClient::TcpClient(const TcpClientOptions& options) :
  Client(options), _ioContext(1), _socket(_ioContext) {
  _serverHost = options._serverHost;
  _tcpPort = options._tcpPort;
}

TcpClient::~TcpClient() {
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool TcpClient::send(const std::vector<char>& subtask) {
  boost::system::error_code ec;
  size_t result[[maybe_unused]] = boost::asio::write(_socket, boost::asio::buffer(subtask), ec);
  if (ec) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ':' << ec.what() << std::endl;
    return false;
  }
  return true;
}

bool TcpClient::receive() {
  char header[HEADER_SIZE + 1] = {};
  memset(header, 0, HEADER_SIZE);
  boost::system::error_code ec;
  boost::asio::read(_socket, boost::asio::buffer(header, HEADER_SIZE), ec);
  auto [uncomprSize, comprSize, compressor, diagnostics, done] =
    decodeHeader(std::string_view(header, HEADER_SIZE), !ec);
  if (!done)
    return false;
  if (!readReply(uncomprSize, comprSize, compressor == COMPRESSORS::LZ4))
    return false;
  return true;
}

bool TcpClient::run() {
  try {
    CloseSocket closeSocket(_socket);
    boost::asio::ip::tcp::resolver resolver(_ioContext);
    boost::system::error_code ec;
    auto endpoint = boost::asio::connect(_socket, resolver.resolve(_serverHost, _tcpPort, ec));
    if (!ec)
      _socket.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
    if (!ec)
      _socket.set_option(boost::asio::socket_base::linger(false, 0), ec);
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " endpoint: " << endpoint << std::endl;
    if (ec) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ':' << ec.what() << std::endl;
      return false;
    }
    return Client::run();
  }
  catch (const std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":exception:" << e.what() << std::endl;
    return false;
  }
  return true;
}

bool TcpClient::readReply(size_t uncomprSize, size_t comprSize, bool bcompressed) {
  std::vector<char>& buffer = _memoryPool.getSecondaryBuffer(comprSize);
  buffer.resize(comprSize);
  boost::system::error_code ec;
  size_t transferred[[maybe_unused]] = boost::asio::read(_socket, boost::asio::buffer(buffer, comprSize), ec);
  if (ec) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << std::endl;
    return false;
  }
  return printReply(buffer, uncomprSize, comprSize, bcompressed);
}

} // end of namespace tcp
