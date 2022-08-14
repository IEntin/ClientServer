/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <exception>
#include <boost/asio.hpp>

struct ClientOptions;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

using DeadlineTimer = boost::asio::deadline_timer;

class TcpHeartbeatClient : public std::enable_shared_from_this<TcpHeartbeatClient>, public Runnable {

  void run() override;
  bool start() override;
  void stop() override;

  void asyncWaitPeriod();

  void asyncWaitTimeout();

  void sendToken();

  void readToken();

  const ClientOptions & _options;

  boost::asio::io_context _ioContext;

  boost::asio::ip::tcp::socket _socket;

  AsioTimer _timerPeriod;

  DeadlineTimer _timerTimeout;

 public:

  TcpHeartbeatClient(const ClientOptions& options);
  ~TcpHeartbeatClient() override;

  char _buffer = '\n';

  static std::atomic<bool> _heartbeatFailed;
};

} // end of namespace tcp
