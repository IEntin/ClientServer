/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"

#include "IOUtility.h"
#include "Options.h"

namespace tcp {

thread_local std::string Tcp::_payload;

bool Tcp::setSocket(boost::asio::ip::tcp::socket& socket) {
  boost::system::error_code ec;
  static const auto ipAdress =
    boost::asio::ip::make_address(Options::_serverAddress, ec);
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
  static const boost::asio::ip::tcp::endpoint endpoint(ipAdress, Options::_tcpPort);
  socket.connect(endpoint, ec);
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

void Tcp::shutdownSocket(boost::asio::ip::tcp::socket& socket) {
  boost::system::error_code ec;
  socket.shutdown(boost::asio::socket_base::shutdown_both, ec);
  if (!ec)
    socket.close(ec);
}

bool Tcp::readMessage(boost::asio::ip::tcp::socket& socket,
		      std::string& payload) {
  boost::system::error_code ec;
  [[maybe_unused]] std::size_t transferred = boost::asio::read_until(socket,
    boost::asio::dynamic_buffer(payload), ENDOFMESSAGE, ec);
  if (ec) {
    Info << ec.what() << '\n';
    return false;
  }
  if (payload.ends_with(ENDOFMESSAGE)) {
    payload.resize(payload.size() - ENDOFMESSAGESZ);     
    return true;
  }
  else {
    LogError << "ENDOFMESSAGE not found\n";
    return false;
  }
}

bool Tcp::readMessage(boost::asio::ip::tcp::socket& socket,
		      HEADER& header,
		      std::span<std::reference_wrapper<std::string>> array) {
  _payload.clear();
  if (!readMessage(socket, _payload))
    return false;
  return ioutility::processMessage(_payload, header, array);
}

} // end of namespace tcp
