/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"
#include "Logger.h"
#include "ClientOptions.h"

namespace tcp {

std::tuple<boost::asio::ip::tcp::endpoint, boost::system::error_code>
Tcp::setSocket(boost::asio::io_context& ioContext,
	       boost::asio::ip::tcp::socket& socket,
	       const ClientOptions& options) {
  boost::asio::ip::tcp::resolver resolver(ioContext);
  boost::system::error_code ec;
  auto endpoint =
    boost::asio::connect(socket, resolver.resolve(options._serverAddress, options._tcpService, ec));
  if (!ec)
    socket.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec)
    LogError << ec.what() << std::endl;
  return { endpoint, ec };
}

std::pair<bool, boost::system::error_code>
Tcp::readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header) {
  // receives interrupt and unblocks read on CtrlC in wait mode
  boost::system::error_code ec;
  socket.wait(boost::asio::ip::tcp::socket::wait_read, ec);
  if (ec) {
    LogError << ec.what() << std::endl;
    return { false, ec };
  }
  char buffer[HEADER_SIZE] = {};
  ec.clear();
  size_t transferred[[maybe_unused]] =
    boost::asio::read(socket, boost::asio::buffer(buffer, HEADER_SIZE), ec);
  if (ec)
    return { false, ec };
  header = decodeHeader(buffer);
  return { true, ec };
}

std::pair<bool, boost::system::error_code>
Tcp::sendMsg(boost::asio::ip::tcp::socket& socket,
	     const HEADER& header,
	     std::string_view body) {
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, header);
  std::vector<boost::asio::const_buffer> buffers;
  buffers.emplace_back(boost::asio::buffer(buffer, HEADER_SIZE));
  if (!body.empty())
    buffers.emplace_back(boost::asio::buffer(body));
  boost::system::error_code ec;
  size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
  if (ec) {
    LogError << ec.what() << std::endl;
    return { false, ec };
  }
  return { true, ec };
}

} // end of namespace tcp
