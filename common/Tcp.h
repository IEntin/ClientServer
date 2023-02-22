/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <boost/asio.hpp>

struct ClientOptions;

namespace tcp {

class Tcp {
  Tcp() = delete;
  ~Tcp() = delete;
public:
  std::tuple<boost::asio::ip::tcp::endpoint, boost::system::error_code>
  static setSocket(boost::asio::io_context& ioContext,
		   boost::asio::ip::tcp::socket& socket,
		   const ClientOptions& options);

  std::pair<bool, boost::system::error_code>
  static readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header);

  std::pair<bool, boost::system::error_code>
  static readMsg(boost::asio::ip::tcp::socket& socket, HEADER& header, std::string& payload);

  std::pair<bool, boost::system::error_code>
  static sendMsg(boost::asio::ip::tcp::socket& socket,
	  const HEADER& header,
	  std::string_view body = std::string_view());
};

} // end of namespace tcp
