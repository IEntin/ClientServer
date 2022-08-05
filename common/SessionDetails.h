/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

namespace tcp {

enum class SESSIONTYPE : char { SESSION = 's', HEARTBEAT = 'h' };

struct SessionDetails {

  SessionDetails() : _ioContext(1), _socket(_ioContext) {}

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::socket _socket;
 };

} // end of namespace tcp
