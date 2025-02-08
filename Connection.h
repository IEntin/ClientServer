/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

namespace tcp {

using ConnectionPtr = std::shared_ptr<struct Connection>;

struct Connection {

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;

  Connection() : _socket(_ioContext) {}
  ~Connection() = default;

};

} // end of namespace tcp
