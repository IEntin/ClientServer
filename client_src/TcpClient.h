/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Client.h"
#include <boost/asio.hpp>

namespace tcp {

class TcpClient : public Client {

  bool send(const Subtask& subtask) override;
  bool receive() override;
  bool receiveStatus() override;
  bool readReply(const HEADER& header);
  bool destroySession() override;

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
  std::string _clientId;
 public:
  TcpClient(const ClientOptions& options);
  ~TcpClient() override;

  bool run() override;
};

} // end of namespace tcp
