/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <boost/asio.hpp>

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
  unsigned getNumberObjects() const override { return 1; }
  unsigned getNumberRunningByType() const override { return 1; }
  void displayCapacityCheck(std::atomic<unsigned>&) const override {}
  void heartbeatWait();
  void timeoutWait();
  void write();
  void read();

  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _periodTimer;
  AsioTimer _timeoutTimer;
  char _heartbeatBuffer[HEADER_SIZE] = {};
};

} // end of namespace tcp
