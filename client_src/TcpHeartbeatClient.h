/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <exception>
#include <boost/asio.hpp>

struct ClientOptions;

namespace tcp {

class TcpHeartbeatClient : public Runnable {

  void run() override;
  bool start() override;
  void stop() override;

  bool heartbeat();

  const ClientOptions & _options;

  std::atomic<bool> _stop = false;

  boost::asio::io_context _ioContext;

  boost::asio::ip::tcp::socket _socket;

 public:

  TcpHeartbeatClient(const ClientOptions& options);
  ~TcpHeartbeatClient() override;

  static std::atomic<bool> _serverDown;
};

} // end of namespace tcp
