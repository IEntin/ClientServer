/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <boost/asio.hpp>

namespace tcp {

std::tuple<boost::asio::ip::tcp::endpoint, boost::system::error_code>
  setSocket(boost::asio::io_context& ioContext,
	    boost::asio::ip::tcp::socket& socket,
	    std::string_view host,
	    std::string_view port);

bool readMsg(boost::asio::ip::tcp::socket& socket,
	     HEADER& header,
	     std::vector<char>& payload,
	     boost::system::error_code ec);

bool sendMsg(boost::asio::ip::tcp::socket& socket,
	     const HEADER& header,
	     std::string_view payload = std::string_view());

} // end of namespace tcp
