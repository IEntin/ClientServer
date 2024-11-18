/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"

#include <array>
#include <cassert>

#include "ClientOptions.h"

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

bool Tcp::readMessage(boost::asio::ip::tcp::socket& socket, std::string& payload) {
  boost::system::error_code ec;
  socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    Warn << ec.what() << '\n';
    return false;
  }
  std::size_t transferred =
    boost::asio::read_until(socket, boost::asio::dynamic_string_buffer(payload), utility::ENDOFMESSAGE, ec);
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
  assert(transferred >= utility::ENDOFMESSAGE.size() + HEADER_SIZE && "Too short message");
  payload.erase(transferred - utility::ENDOFMESSAGE.size());
  return true;
}

} // end of namespace tcp
