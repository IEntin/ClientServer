/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

#include "Header.h"
#include "Logger.h"

namespace tcp {

class Tcp {
  Tcp() = delete;
  ~Tcp() = delete;

  boost::system::error_code
  static readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header);
public:
  std::tuple<boost::asio::ip::tcp::endpoint, boost::system::error_code>
  static setSocket(boost::asio::ip::tcp::socket& socket);

  static boost::system::error_code
  readMsg(boost::asio::ip::tcp::socket& socket, HEADER& header, std::string& payload);

  static boost::system::error_code sendMsg(boost::asio::ip::tcp::socket& socket,
					   const HEADER& header,
					   std::string_view body = {});
};

} // end of namespace tcp
