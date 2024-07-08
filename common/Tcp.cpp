/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"

#include <array>

#include "ClientOptions.h"
#include "Logger.h"

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

void Tcp::sendMsg(boost::asio::ip::tcp::socket& socket, std::string_view payload) {
  char sizeStr[ioutility::CONV_BUFFER_SIZE] = {};
  ioutility::toChars(payload.size(), sizeStr);
  std::array<boost::asio::const_buffer, 2>
    buffers{boost::asio::buffer(sizeStr), boost::asio::buffer(payload)};
  std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers);
}

} // end of namespace tcp
