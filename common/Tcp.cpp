/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"
#include "ClientOptions.h"
#include "Utility.h"

namespace tcp {

CloseSocket::CloseSocket(boost::asio::ip::tcp::socket& socket) : _socket(socket) {}

CloseSocket::~CloseSocket() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
}

std::tuple<boost::asio::ip::tcp::endpoint, boost::system::error_code>
setSocket(boost::asio::io_context& ioContext,
	  boost::asio::ip::tcp::socket& socket,
	  std::string_view host,
	  std::string_view port) {
  boost::asio::ip::tcp::resolver resolver(ioContext);
  boost::system::error_code ec;
  auto endpoint = boost::asio::connect(socket, resolver.resolve(host, port, ec));
  if (!ec)
    socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (!ec)
    socket.set_option(boost::asio::socket_base::linger(false, 0), ec);
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
  return { endpoint, ec };
}

HEADER receiveHeader(boost::asio::ip::tcp::socket& socket) {
  char buffer[HEADER_SIZE] = {};
  boost::system::error_code ec;
  boost::asio::read(socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return { 0, 0, COMPRESSORS::NONE, false, 0, 0, PROBLEMS::TCP_PROBLEM };
  }
  return decodeHeader(std::string_view(buffer, HEADER_SIZE));
}

bool heartbeat(const ClientOptions& options) {
  boost::asio::io_context ioContext;
  boost::asio::ip::tcp::socket socket(ioContext);
  CloseSocket closeSocket(socket);
  auto [endpoint, error] =
    setSocket(ioContext, socket, options._serverHost, options._tcpHeartbeatPort);
  if (error)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << error.what() << '\n';
  boost::system::error_code ec;
  char data = '\n';
  size_t transferred[[maybe_unused]] = boost::asio::write(socket, boost::asio::buffer(&data, 1), ec);
  if (!ec) {
    data = '\0';
    transferred = boost::asio::read(socket, boost::asio::buffer(&data, 1), ec);
  }
  if (ec)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
  return !ec;
}

} // end of namespace tcp
