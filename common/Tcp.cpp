/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"

#include <array>

#include "ClientOptions.h"

namespace tcp {

boost::system::error_code
Tcp::setSocket(boost::asio::ip::tcp::socket& socket) {
  boost::system::error_code ecc;
  if (socket.is_open())
    socket.close(ecc);
  if (ecc)
    Warn << ecc.what() << '\n';
  boost::system::error_code ec;
  static const auto ipAdress =
    boost::asio::ip::address::from_string(ClientOptions::_serverAddress);
  static const boost::asio::ip::tcp::endpoint endpoint(ipAdress, ClientOptions::_tcpPort);
  socket.connect(endpoint, ec);
  if (!ec)
    socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
  else
    LogError << ec.what() << '\n';
  return ec;
}

boost::system::error_code
Tcp::readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header) {
  // catch signal and unblock read in wait mode
  boost::system::error_code ec;
  socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return ec;
  }
  char buffer[HEADER_SIZE] = {};
  std::size_t transferred[[maybe_unused]] =
    boost::asio::read(socket, boost::asio::buffer(buffer), ec);
  if (ec)
    return ec;
  header = decodeHeader(buffer);
  return ec;
}

boost::system::error_code
Tcp::readMsg(boost::asio::ip::tcp::socket& socket, HEADER& header, std::string& payload) {
  auto ec = readHeader(socket, header);
  if (ec)
    return ec;
  std::size_t size = extractPayloadSize(header);
  if (size > 0) {
    payload.resize(size);
    boost::system::error_code ec;
    std::size_t transferred[[maybe_unused]] = boost::asio::read(socket, boost::asio::buffer(payload), ec);
  }
  return ec;
}

boost::system::error_code
Tcp::sendMsg(boost::asio::ip::tcp::socket& socket, const HEADER& header, std::string_view body) {
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, header);
  std::array<boost::asio::const_buffer, 2> buffers{boost::asio::buffer(buffer), boost::asio::buffer(body)};
  boost::system::error_code ec;
  std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
  if (ec)
    LogError << ec.what() << '\n';
  return ec;
}

boost::system::error_code
Tcp::sendMsg(boost::asio::ip::tcp::socket& socket, std::string_view payload) {
  char sizeStr[ioutility::CONV_BUFFER_SIZE] = {};
  ioutility::toChars(payload.size(), sizeStr);
  std::array<boost::asio::const_buffer, 2>
    buffers{boost::asio::buffer(sizeStr), boost::asio::buffer(payload)};
  boost::system::error_code ec;
  std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
  if (ec)
    LogError << ec.what() << '\n';
  return ec;
}

} // end of namespace tcp
