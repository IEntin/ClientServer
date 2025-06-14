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

  static std::vector<unsigned char> _defaultParameter;
  static thread_local std::string _payload;

public:

  static void shutdownSocket(boost::asio::ip::tcp::socket& socket);

  static bool setSocket(boost::asio::ip::tcp::socket& socket);

  static bool readMessage(boost::asio::ip::tcp::socket& socket,
			  HEADER& header,
			  std::string& payload1,
			  std::vector<unsigned char>& payload2 = Tcp::_defaultParameter);

  template <typename P1, typename P2, typename P3>
  static bool readMessage(boost::asio::ip::tcp::socket& socket,
			  HEADER& header,
			  P1& payload1,
			  P2& payload2,
			  P3& payload3) {
    _payload.clear();
    if (!readMessage(socket, _payload))
      return false;
    deserialize(header, _payload.data());
    std::size_t payload1Sz = extractReservedSz(header);
    std::size_t payload2Sz = extractUncompressedSize(header);
    std::size_t payload3Sz = extractParameter(header);
    payload1.resize(payload1Sz);
    payload2.resize(payload2Sz);
    payload3.resize(payload3Sz);
    unsigned shift = HEADER_SIZE;
    if (payload1Sz > 0)
      std::copy(_payload.cbegin() + shift, _payload.cbegin() + shift + payload1Sz, payload1.begin());
    shift += payload1Sz;
    if (payload2Sz > 0)
      std::copy(_payload.cbegin() + shift, _payload.cbegin() + shift + payload2Sz, payload2.begin());
    shift += payload2Sz;
    if (payload3Sz > 0)
      std::copy(_payload.cbegin() + shift, _payload.cbegin() + shift + payload3Sz, payload3.begin());
    return true;
  }

  template <typename P1 = std::span<const char>, typename P2 = P1, typename P3 = P2>
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
      switch (ec.value()) {
      case boost::asio::error::eof:
      case boost::asio::error::connection_refused:
      case boost::asio::error::connection_reset:
      case boost::asio::error::broken_pipe:
	Info << ec.what() << '\n';
	break;
      default:
	Warn << ec.what() << '\n';
	return false;
      }
    }
    return true;
  }

  static bool readMessage(boost::asio::ip::tcp::socket& socket,
  			  std::string& payload);
};

} // end of namespace tcp
