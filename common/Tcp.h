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

  static void readHeader(boost::asio::ip::tcp::socket& socket, HEADER& header);
public:
  static void setSocket(boost::asio::ip::tcp::socket& socket);

  static void readMsg(boost::asio::ip::tcp::socket& socket, HEADER& header, std::string& payload);

  static void sendMsg(boost::asio::ip::tcp::socket& socket,
		      const HEADER& header,
		      std::string_view body = {});

  static void sendMsg(boost::asio::ip::tcp::socket& socket, std::string_view payload);
};

} // end of namespace tcp
