/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

#include "Client.h"

namespace tcp {

class TcpClient : public Client {

  bool send(Subtask& subtask) override;
  bool receive() override;
  bool receiveStatus() override;

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
 public:
  TcpClient();
  ~TcpClient() override;

  void run() override;
};

} // end of namespace tcp
