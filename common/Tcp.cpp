/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"

#include <array>
#include <cassert>

#include "ClientOptions.h"
#include "Utility.h"

namespace tcp {

bool Tcp::setSocket(boost::asio::ip::tcp::socket& socket) {
  boost::system::error_code ec;
  static const auto ipAdress =
    boost::asio::ip::make_address(ClientOptions::_serverAddress, ec);
  if (ec) {
    switch (ec.value()) {
    case boost::asio::error::connection_refused:
      Info << ec.what() << '\n';
      return false;
    default:
      LogError << ec.what() << '\n';
      return false;
    }
  }

  static const boost::asio::ip::tcp::endpoint endpoint(ipAdress, ClientOptions::_tcpPort);
  socket.connect(endpoint, ec);
  if (ec) {
    LogError << ec.what() << '\n';
    return false;
  }
  socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    if (ec.value() == boost::asio::error::connection_reset)
      Info << ec.what() << '\n';
    return false;
  }
  return true;
}

bool Tcp::sendMessage(boost::asio::ip::tcp::socket& socket, std::string_view payload) {
  std::array<boost::asio::const_buffer, 2>
    buffers{ boost::asio::buffer(payload),
	     boost::asio::buffer(utility::ENDOFMESSAGE) };
  boost::system::error_code ec;
  socket.wait(boost::asio::ip::tcp::socket::wait_write, ec);
  if (ec) {
    Warn << ec.what() << '\n';
    return false;
  }
  std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
  if (ec) {
    switch (ec.value()) {
    case boost::asio::error::eof:
    case boost::asio::error::connection_refused:
    case boost::asio::error::connection_reset:
    case boost::asio::error::broken_pipe:
      Info << ec.what() << '\n';
      return false;
    default:
      Warn << ec.what() << '\n';
      return false;
    }
  }
  return true;
}

bool Tcp::readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header) {
  boost::system::error_code ec;
  socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    Warn << ec.what() << '\n';
    return false;
  }
  char buffer[HEADER_SIZE] = {};
  std::size_t transferred [[maybe_unused]] =
    boost::asio::read(socket, boost::asio::buffer(buffer), ec);
  if (ec) {
    switch (ec.value()) {
    case boost::asio::error::eof:
    case boost::asio::error::connection_reset:
    case boost::asio::error::broken_pipe:
      Info << ec.what() << '\n';
      return false;
    default:
      Warn << ec.what() << '\n';
      return false;
    }
  }
  return deserialize(header, buffer);  
}

bool Tcp::readMessage(boost::asio::ip::tcp::socket& socket,
		      std::string& payload,
		      std::size_t size) {
  boost::system::error_code ec;
  socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    Warn << ec.what() << '\n';
    return false;
  }
  std::size_t transferred [[maybe_unused]] =
    boost::asio::read(socket, boost::asio::buffer(payload, size), ec);
  if (ec) {
    switch (ec.value()) {
    case boost::asio::error::eof:
    case boost::asio::error::connection_reset:
    case boost::asio::error::broken_pipe:
      Info << ec.what() << '\n';
      return false;
    default:
      Warn << ec.what() << '\n';
      return false;
    }
  }
  return true;
}

} // end of namespace tcp
