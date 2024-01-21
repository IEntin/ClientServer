/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

#include "Client.h"

namespace tcp {

class TcpClient : public Client {

  bool sendBString();
  bool send(const Subtask& subtask) override;
  bool receive() override;
  bool receiveStatus() override;
  void close() override;

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
  std::string _clientId;
  std::string _response;
 public:
  TcpClient();
  ~TcpClient() override;

  bool run() override;
};

} // end of namespace tcp
