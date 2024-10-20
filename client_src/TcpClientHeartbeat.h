/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/asio.hpp>

#include "Runnable.h"

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class TcpClientHeartbeat final : public std::enable_shared_from_this<TcpClientHeartbeat>,
  public Runnable {
 public:
  TcpClientHeartbeat();
  ~TcpClientHeartbeat() override;
 private:
  void run() override;
  bool start() override;
  void stop() override;
  void heartbeatWait();
  void timeoutWait();
  void write();
  void read();

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _periodTimer;
  AsioTimer _timeoutTimer;
  std::string _heartbeatBuffer;
};

} // end of namespace tcp
