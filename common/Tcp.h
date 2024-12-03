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
    if (!readHeader(socket, header))
      return false;
    static thread_local std::string payload;
    std::size_t size = extractUncompressedSize(header);
    if (size != 0) {
      payload.resize(size);
      if (!readMessage(socket, payload, size))
	return false;
      payload1.resize(size);
      std::memcpy(payload1.data(), payload.data(), size);
    }
    size = extractParameter(header);
    if (size != 0) {
      payload.resize(size);
      if (!readMessage(socket, payload, size))
	return false;
      payload2.resize(size);
      std::memcpy(payload2.data(), payload.data(), size);
    }
    return true;
  }

  template <typename P1 = std::vector<char>, typename P2 = P1>
  static bool sendMsg(boost::asio::ip::tcp::socket& socket,
		      const HEADER& header,
		      const P1& payload1 = P1(),
		      const P2& payload2 = P2()) {
    char headerBuffer[HEADER_SIZE] = {};
    serialize(header, headerBuffer);
    std::array<boost::asio::const_buffer, 3> buffers{ boost::asio::buffer(headerBuffer),
						      boost::asio::buffer(payload1),
						      boost::asio::buffer(payload2) };
    boost::system::error_code ec;
    std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
    if (ec) {
      LogError << ec.what() << '\n';
      return false;
    }
    return true;
  }

  template <typename P1 = std::vector<char>, typename P2 = P1>
  static bool sendMsgNE(boost::asio::ip::tcp::socket& socket,
			const HEADER& header,
			const P1& payload1 = P1(),
			const P2& payload2 = P2()) {
    char headerBuffer[HEADER_SIZE] = {};
    serialize(header, headerBuffer);
    std::array<boost::asio::const_buffer, 4> buffers{ boost::asio::buffer(headerBuffer),
						      boost::asio::buffer(payload1),
						      boost::asio::buffer(payload2) };
    boost::system::error_code ec;
    std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
    if (ec) {
      LogError << ec.what() << '\n';
      return false;
    }
    return true;
  }

  static bool readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header);
  static bool sendMessage(boost::asio::ip::tcp::socket& socket, std::string_view payload);
  static bool readMessage(boost::asio::ip::tcp::socket& socket,
			  std::string& payload,
			  std::size_t size);
};

} // end of namespace tcp
