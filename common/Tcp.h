/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

#include "IOUtility.h"
#include "Logger.h"

namespace tcp {

class Tcp {
  Tcp() = delete;
  ~Tcp() = delete;

public:

  static void shutdownSocket(boost::asio::ip::tcp::socket& socket) {
    boost::system::error_code ec;
    socket.shutdown(boost::asio::socket_base::shutdown_both, ec);
    if (!ec)
      socket.close(ec);
  }

  static bool setSocket(boost::asio::ip::tcp::socket& socket);

  template <typename P1, typename P2 = P1>
  static bool readMsg(boost::asio::ip::tcp::socket& socket,
		      HEADER& header,
		      P1& payload1,
		      P2&& payload2 = P2()) {
    static thread_local std::string payload;
    payload.erase(0);
    boost::system::error_code ec;
    std::size_t transferred =
      boost::asio::read_until(socket, boost::asio::dynamic_string_buffer(payload), ioutility::ENDOFMESSAGE, ec);
    if (ec) {
      LogError << ec.what() << '\n';
      return false;
    }
    transferred -= ioutility::ENDOFMESSAGE.size();
    if (!deserialize(header, payload.data()))
      return false;
    std::size_t payload2Size = extractParameter(header);
    std::size_t payload1Size = transferred - HEADER_SIZE - payload2Size;
    payload1 = { reinterpret_cast<decltype(payload1.data())>(payload.data()) + HEADER_SIZE, payload1Size };
    payload2 = { reinterpret_cast<decltype(payload2.data())>(payload.data()) + HEADER_SIZE + payload1Size, payload2Size };
    return true;
  }

  template <typename P1 = std::vector<char>, typename P2 = P1>
  static bool sendMsg(boost::asio::ip::tcp::socket& socket,
		      const HEADER& header,
		      const P1& payload1 = P1(),
		      const P2& payload2 = P2()) {
    char headerBuffer[HEADER_SIZE] = {};
    serialize(header, headerBuffer);
    std::array<boost::asio::const_buffer, 4> buffers{ boost::asio::buffer(headerBuffer),
						      boost::asio::buffer(payload1),
						      boost::asio::buffer(payload2),
						      boost::asio::buffer(ioutility::ENDOFMESSAGE) };
    boost::system::error_code ec;
    std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
    if (ec) {
      LogError << ec.what() << '\n';
      return false;
    }
    return true;
  }

  static bool sendMessage(boost::asio::ip::tcp::socket& socket, std::string_view body);
  static bool readMessage(boost::asio::ip::tcp::socket& socket, std::string& payload);
};

} // end of namespace tcp
