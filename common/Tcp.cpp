/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"
#include "ClientOptions.h"

namespace tcp {

std::tuple<boost::asio::ip::tcp::endpoint, boost::system::error_code>
Tcp::setSocket(boost::asio::io_context& ioContext,
	       boost::asio::ip::tcp::socket& socket) {
  boost::asio::ip::tcp::resolver resolver(ioContext);
  boost::system::error_code ec;
  auto endpoint =
    boost::asio::connect(socket, resolver.resolve(ClientOptions::_serverAddress, ClientOptions::_tcpService, ec));
  if (!ec)
    socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec)
    LogError << ec.what() << '\n';
  return { endpoint, ec };
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
  size_t transferred[[maybe_unused]] =
    boost::asio::read(socket, boost::asio::buffer(buffer), ec);
  if (ec)
    return ec;
  header = decodeHeader(buffer);
  return ec;
}

} // end of namespace tcp
