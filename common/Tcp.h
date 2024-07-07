/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>
#include <cryptopp/secblock.h>

#include "Header.h"

namespace tcp {

class Tcp {
  Tcp() = delete;
  ~Tcp() = delete;

  static void readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header);
public:
  static void setSocket(boost::asio::ip::tcp::socket& socket);

  static void readMsg(boost::asio::ip::tcp::socket& socket, HEADER& header, std::string& payload);

  template <typename B>
  static void sendMsg(boost::asio::ip::tcp::socket& socket,
		      const HEADER& header,
		      B& body) {
    char buffer[HEADER_SIZE] = {};
    encodeHeader(buffer, header);
    std::array<boost::asio::const_buffer, 2> buffers{boost::asio::buffer(buffer), boost::asio::buffer(body)};
    std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers);
  }

  static void sendMsg(boost::asio::ip::tcp::socket& socket, std::string_view payload);
};

} // end of namespace tcp
