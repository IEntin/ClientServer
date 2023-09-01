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

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
  std::string _clientId;
  RunnableWeakPtr _signalWatcher;
  std::string _response;
 public:
  TcpClient();
  ~TcpClient() override;

  bool run() override;

  void createSignalWatcher();
};

} // end of namespace tcp
