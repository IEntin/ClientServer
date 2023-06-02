/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

namespace tcp {

struct ConnectionDetails {

  ConnectionDetails() : _ioContext(1), _socket(_ioContext) {}
  ~ConnectionDetails() = default;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
};

} // end of namespace tcp
