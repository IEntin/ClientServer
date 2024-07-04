/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"

#include <array>

#include "ClientOptions.h"

namespace tcp {

void Tcp::setSocket(boost::asio::ip::tcp::socket& socket) {
  static const auto ipAdress =
    boost::asio::ip::address::from_string(ClientOptions::_serverAddress);
  static const boost::asio::ip::tcp::endpoint endpoint(ipAdress, ClientOptions::_tcpPort);
  socket.connect(endpoint);
  socket.set_option(boost::asio::socket_base::reuse_address(true));
}

void Tcp::readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header) {
  // catch signal and unblock read in wait mode
  socket.wait(boost::asio::ip::tcp::socket::wait_read);
  char buffer[HEADER_SIZE] = {};
  std::size_t transferred[[maybe_unused]] =
    boost::asio::read(socket, boost::asio::buffer(buffer));
  header = decodeHeader(buffer);
}

void Tcp::readMsg(boost::asio::ip::tcp::socket& socket, HEADER& header, std::string& payload) {
  readHeader(socket, header);
  std::size_t size = extractPayloadSize(header);
  if (size > 0) {
    payload.resize(size);
    boost::system::error_code ec;
    std::size_t transferred[[maybe_unused]] = boost::asio::read(socket, boost::asio::buffer(payload), ec);
  }
}

void Tcp::sendMsg(boost::asio::ip::tcp::socket& socket, const HEADER& header, std::string_view body) {
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, header);
  std::array<boost::asio::const_buffer, 2> buffers{boost::asio::buffer(buffer), boost::asio::buffer(body)};
  std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers);
}

void Tcp::sendMsg(boost::asio::ip::tcp::socket& socket, std::string_view payload) {
  char sizeStr[ioutility::CONV_BUFFER_SIZE] = {};
  ioutility::toChars(payload.size(), sizeStr);
  std::array<boost::asio::const_buffer, 2>
    buffers{boost::asio::buffer(sizeStr), boost::asio::buffer(payload)};
  std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers);
}

} // end of namespace tcp
