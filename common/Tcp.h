/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <boost/asio.hpp>

namespace tcp {

struct CloseSocket {
  CloseSocket(boost::asio::ip::tcp::socket& socket);
  ~CloseSocket();
  boost::asio::ip::tcp::socket& _socket;
};

bool setSocket(boost::asio::io_context& ioContext,
	       boost::asio::ip::tcp::socket& socket,
	       std::string_view host,
	       std::string_view port);

HEADER receiveHeader(boost::asio::ip::tcp::socket& socket);

} // end of namespace tcp
