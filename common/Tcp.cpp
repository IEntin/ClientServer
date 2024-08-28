/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"

#include <array>

#include "ClientOptions.h"
#include "IOUtility.h"

namespace tcp {

bool Tcp::setSocket(boost::asio::ip::tcp::socket& socket) {
  static const auto ipAdress =
    boost::asio::ip::address::from_string(ClientOptions::_serverAddress);
  static const boost::asio::ip::tcp::endpoint endpoint(ipAdress, ClientOptions::_tcpPort);
  boost::system::error_code ec;
  socket.connect(endpoint, ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  return true;
}

bool Tcp::readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header) {
  // catch signal and unblock read in wait mode
  boost::system::error_code ec;
  socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  char buffer[HEADER_SIZE] = {};
  std::size_t transferred[[maybe_unused]] =
    boost::asio::read(socket, boost::asio::buffer(buffer), ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  deserialize(header, buffer);
  return true;
}

bool Tcp::sendMessage(boost::asio::ip::tcp::socket& socket, const HEADER& header, std::string_view body) {
  std::size_t payloadSize = HEADER_SIZE + body.size();
  char sizeStr[ioutility::CONV_BUFFER_SIZE] = {};
  ioutility::toChars(payloadSize, sizeStr);
  char headerBuffer[HEADER_SIZE] = {};
  serialize(header, headerBuffer);
  std::array<boost::asio::const_buffer, 3>
    buffers{boost::asio::buffer(sizeStr),
	    boost::asio::buffer(headerBuffer),
	    boost::asio::buffer(body)};
  boost::system::error_code ec;
  std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  return bytes == payloadSize;
}

bool Tcp::sendMessage(boost::asio::ip::tcp::socket& socket, std::string_view body) {
  std::size_t payloadSize = body.size();
  char sizeStr[ioutility::CONV_BUFFER_SIZE] = {};
  ioutility::toChars(payloadSize, sizeStr);
  std::array<boost::asio::const_buffer, 2>
    buffers{boost::asio::buffer(sizeStr),
	    boost::asio::buffer(body)};
  boost::system::error_code ec;
  std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  return bytes == payloadSize;
}

bool Tcp::readMessage(boost::asio::ip::tcp::socket& socket, std::string& payload) {
  boost::system::error_code ec;
  socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  char buffer[ioutility::CONV_BUFFER_SIZE] = {};
  std::size_t transferred[[maybe_unused]] =
    boost::asio::read(socket, boost::asio::buffer(buffer), ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  std::size_t payloadSize = 0;
  ioutility::fromChars(buffer, payloadSize);
  payload.resize(payloadSize);
  transferred = boost::asio::read(socket, boost::asio::buffer(payload), ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  return transferred == payloadSize;
}

} // end of namespace tcp
