/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"
#include "Utility.h"

namespace tcp {

CloseSocket::CloseSocket(boost::asio::ip::tcp::socket& socket) : _socket(socket) {}

CloseSocket::~CloseSocket() {
  boost::system::error_code ignore;
  _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
  _socket.close(ignore);
}

bool setSocket(boost::asio::io_context& ioContext,
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
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " endpoint: " << endpoint << '\n';
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return false;
  }
  return true;
}

bool sendHeader(boost::asio::ip::tcp::socket& socket, HEADER header){
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer,
	       HEADER_SIZE,
	       HEADER_SIZE,
	       getCompressor(header),
	       isDiagnosticsEnabled(header),
	       getEphemeral(header),
	       getProblem(header));
  boost::system::error_code ec;
  size_t result[[maybe_unused]] =
    boost::asio::write(socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ':' << ec.what() << std::endl;
    return false;
  }
  return true;
}

HEADER receiveHeader(boost::asio::ip::tcp::socket& socket) {
  char buffer[HEADER_SIZE] = {};
  boost::system::error_code ec;
  boost::asio::read(socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << ec.what() << '\n';
    return { -1, -1, COMPRESSORS::NONE, false, 0, PROBLEMS::BAD_HEADER };
  }
  return decodeHeader(std::string_view(buffer, HEADER_SIZE));
}

} // end of namespace tcp
