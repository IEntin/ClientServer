/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

#include "Header.h"
#include "Utility.h"

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

  template <typename P1>
  static bool readMsg(boost::asio::ip::tcp::socket& socket,
		      HEADER& header,
		      P1& payload1) {
    static thread_local std::string payload;
    payload.clear();
    if (!readMessage(socket, payload))
      return false;
    deserialize(header, payload.data());
    std::size_t payload1Sz = extractUncompressedSize(header);
    payload1.resize(payload1Sz);
    unsigned shift = HEADER_SIZE;
    if (payload1Sz > 0)
      std::memcpy(payload1.data(), payload.data() + shift, payload1Sz);
    return true;
  }

  template <typename P1, typename P2>
  static bool readMsg(boost::asio::ip::tcp::socket& socket,
		      HEADER& header,
		      P1& payload1,
		      P2& payload2) {
    static thread_local std::string payload;
    payload.clear();
    if (!readMessage(socket, payload))
      return false;
    deserialize(header, payload.data());
    std::size_t payload1Sz = extractUncompressedSize(header);
    std::size_t payload2Sz = extractParameter(header);
    payload1.resize(payload1Sz);
    payload2.resize(payload2Sz);
    unsigned shift = HEADER_SIZE;
    if (payload1Sz > 0)
      std::memcpy(payload1.data(), payload.data() + shift, payload1Sz);
    shift += payload1Sz;
    if (payload2Sz > 0)
      std::memcpy(payload2.data(), payload.data() + shift, payload2Sz);
    return true;
  }

  template <typename P1, typename P2, typename P3>
  static bool readMsg(boost::asio::ip::tcp::socket& socket,
		      HEADER& header,
		      P1& payload1,
		      P2& payload2,
		      P3& payload3) {
    static thread_local std::string payload;
    payload.clear();
    if (!readMessage(socket, payload))
      return false;
    deserialize(header, payload.data());
    std::size_t payload1Sz = extractReservedSz(header);
    std::size_t payload2Sz = extractUncompressedSize(header);
    std::size_t payload3Sz = extractParameter(header);
    payload1.resize(payload1Sz);
    payload2.resize(payload2Sz);
    payload3.resize(payload3Sz);
    unsigned shift = HEADER_SIZE;
    if (payload1Sz > 0)
      std::memcpy(payload1.data(), payload.data() + shift, payload1Sz);
    shift += payload1Sz;
    if (payload2Sz > 0)
      std::memcpy(payload2.data(), payload.data() + shift, payload2Sz);
    shift += payload2Sz;
    if (payload3Sz > 0)
      std::memcpy(payload3.data(), payload.data() + shift, payload3Sz);
    return true;
  }

  template <typename P1 = std::vector<char>, typename P2 = P1, typename P3 = P2>
  static bool sendMsg(boost::asio::ip::tcp::socket& socket,
		      const HEADER& header,
		      const P1& payload1 = P1(),
		      const P2& payload2 = P2(),
		      const P3& payload3 = P3()) {
    char headerBuffer[HEADER_SIZE] = {};
    serialize(header, headerBuffer);
    std::array<boost::asio::const_buffer, 5> buffers{ boost::asio::buffer(headerBuffer),
						      boost::asio::buffer(payload1),
						      boost::asio::buffer(payload2),
						      boost::asio::buffer(payload3),
						      boost::asio::buffer(ENDOFMESSAGE) };
    boost::system::error_code ec;
    std::size_t bytes[[maybe_unused]] = boost::asio::write(socket, buffers, ec);
    if (ec) {
      LogError << ec.what() << '\n';
      return false;
    }
    return true;
  }

  static bool sendMessage(boost::asio::ip::tcp::socket& socket, std::string_view payload);
  static bool readMessage(boost::asio::ip::tcp::socket& socket,
			  std::string& payload);
};

} // end of namespace tcp
