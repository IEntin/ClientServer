/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Logger.h"
#include <boost/asio.hpp>

namespace tcp {

class Tcp {
  Tcp() = delete;
  ~Tcp() = delete;

  boost::system::error_code
  static readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header);
public:
  std::tuple<boost::asio::ip::tcp::endpoint, boost::system::error_code>
  static setSocket(boost::asio::io_context& ioContext,
		   boost::asio::ip::tcp::socket& socket);

  template <typename P>
  static boost::system::error_code
  readMsg(boost::asio::ip::tcp::socket& socket, HEADER& header, P& payload) {
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

  template <typename P>
  boost::system::error_code
  static sendMsg(boost::asio::ip::tcp::socket& socket,
		 const HEADER& header,
		 const P &body) {
    char buffer[HEADER_SIZE] = {};
    encodeHeader(buffer, header);
    std::vector<boost::asio::const_buffer> buffers;
    buffers.emplace_back(boost::asio::buffer(buffer));
    if (!body.empty())
      buffers.emplace_back(boost::asio::buffer(body));
    boost::system::error_code ec;
    size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
    if (ec)
      LogError << ec.what() << '\n';
    return ec;
  }
};

} // end of namespace tcp
