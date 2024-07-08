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

  template <typename B>
  static void readMsg(boost::asio::ip::tcp::socket& socket, HEADER& header, B& payload) {
    readHeader(socket, header);
    std::size_t size = extractPayloadSize(header);
    if (size > 0) {
      payload.resize(size);
      std::size_t transferred[[maybe_unused]] = boost::asio::read(socket, boost::asio::buffer(payload));
    }
  }

  static void readMsg(boost::asio::ip::tcp::socket& socket,
		      HEADER& header,
		      std::string& payload1,
		      CryptoPP::SecByteBlock& payload2);

  template <typename B>
  static void sendMsg(boost::asio::ip::tcp::socket& socket,
		      const HEADER& header,
		      B& body) {
    char buffer[HEADER_SIZE] = {};
    encodeHeader(buffer, header);
    std::array<boost::asio::const_buffer, 2> buffers{boost::asio::buffer(buffer), boost::asio::buffer(body)};
    std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers);
  }

  static void sendMsg(boost::asio::ip::tcp::socket& socket,
		      const HEADER& header,
		      std::string_view payload1,
		      CryptoPP::SecByteBlock& payload2);

  static void sendMsg(boost::asio::ip::tcp::socket& socket, std::string_view payload);
};

} // end of namespace tcp
