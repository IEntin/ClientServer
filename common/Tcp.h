/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

#include "Header.h"

namespace tcp {

class Tcp {
  Tcp() = delete;
  ~Tcp() = delete;

  static void readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header);
public:
  static void setSocket(boost::asio::ip::tcp::socket& socket);

  template <typename P1, typename P2 = P1>
  static void readMsg(boost::asio::ip::tcp::socket& socket,
		      HEADER& header,
		      P1& payload1,
		      P2&& payload2 = P2()) {
    readHeader(socket, header);
    std::size_t payloadSize1 = extractPayloadSize(header);
    payload1.resize(payloadSize1);
    std::size_t transferred[[maybe_unused]] = boost::asio::read(socket, boost::asio::buffer(payload1));
    std::size_t payload2Size = extractParameter(header);
    if (payload2Size > 0) {
      payload2.resize(payload2Size);
      transferred = boost::asio::read(socket, boost::asio::buffer(payload2));
    }
  }

  template <typename P1 = std::vector<char>, typename P2 = P1>
  static void sendMsg(boost::asio::ip::tcp::socket& socket,
		      const HEADER& header,
		      const P1& payload1 = P1(),
		      const P2& payload2 = P2()) {
    char headerBuffer[HEADER_SIZE] = {};
    serialize(header, headerBuffer);
    std::array<boost::asio::const_buffer, 3> buffers{ boost::asio::buffer(headerBuffer),
						      boost::asio::buffer(payload1),
						      boost::asio::buffer(payload2) };
    std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers);
  }

  static bool sendMessage(boost::asio::ip::tcp::socket& socket, const HEADER& header, std::string_view body);
  static bool sendMessage(boost::asio::ip::tcp::socket& socket, std::string_view body);
  static bool readMessage(boost::asio::ip::tcp::socket& socket, std::string& payload);
};

} // end of namespace tcp
