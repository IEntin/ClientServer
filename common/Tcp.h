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

  static thread_local std::string _payload;

public:

  static void shutdownSocket(boost::asio::ip::tcp::socket& socket);

  static bool setSocket(boost::asio::ip::tcp::socket& socket);

  static bool readMessage(boost::asio::ip::tcp::socket& socket,
			  HEADER& header,
			  std::string& payload);

  static bool readMessage(boost::asio::ip::tcp::socket& socket,
			  HEADER& header,
			  std::string& payload1,
			  std::string& payload2);

  static bool readMessage(boost::asio::ip::tcp::socket& socket,
			  HEADER& header,
			  std::string& payload1,
			  std::string& payload2,
			  std::vector<unsigned char>& payload3);

  template <typename P1 = std::span<const char>, typename P2 = P1, typename P3 = P1>
  static bool sendMessage(boost::asio::ip::tcp::socket& socket,
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
