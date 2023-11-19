/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Tcp.h"

#include <boost/container/static_vector.hpp>

#include "ClientOptions.h"

namespace tcp {

std::tuple<boost::asio::ip::tcp::endpoint, boost::system::error_code>
Tcp::setSocket(boost::asio::ip::tcp::socket& socket) {
  boost::system::error_code ignore;
  socket.close(ignore);
  boost::system::error_code ec;
  static const auto ipAdress =
    boost::asio::ip::address::from_string(ClientOptions::_serverAddress);
  static const boost::asio::ip::tcp::endpoint endpoint(ipAdress, ClientOptions::_tcpPort);
  socket.connect(endpoint, ec);
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

boost::system::error_code
Tcp::readMsg(boost::asio::ip::tcp::socket& socket, HEADER& header, std::string& payload) {
  auto ec = readHeader(socket, header);
  if (ec)
    return ec;
  size_t size = extractPayloadSize(header);
  if (size > 0) {
    payload.resize(size);
    boost::system::error_code ec;
    size_t transferred[[maybe_unused]] = boost::asio::read(socket, boost::asio::buffer(payload), ec);
  }
  return ec;
}

boost::system::error_code
Tcp::sendMsg(boost::asio::ip::tcp::socket& socket, const HEADER& header, std::string_view body) {
    char buffer[HEADER_SIZE] = {};
    encodeHeader(buffer, header);
    static thread_local boost::container::static_vector<boost::asio::const_buffer, 2> buffers;
    buffers.clear();
    buffers.emplace_back(boost::asio::buffer(buffer));
    if (!body.empty())
      buffers.emplace_back(boost::asio::buffer(body));
    boost::system::error_code ec;
    size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
    if (ec)
      LogError << ec.what() << '\n';
    return ec;
  }

} // end of namespace tcp
