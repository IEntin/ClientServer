/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Runnable.h"
#include <boost/asio.hpp>

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class TcpSession;

using TcpSessionPtr = std::shared_ptr<TcpSession>;

using TcpServerPtr = std::shared_ptr<class TcpServer>;

class TcpHeartbeat : public std::enable_shared_from_this<TcpHeartbeat>, public Runnable {
 public:
  TcpHeartbeat(TcpSessionPtr session, TcpServerPtr parent);
  ~TcpHeartbeat() override;

  bool start() override;
  void stop() override {}

 private:

  void run() noexcept override;

  void heartbeatWait();

  void heartbeat();

  TcpSessionPtr _session;
  TcpServerPtr _parent;
  std::atomic<PROBLEMS>& _problem;
  boost::asio::io_context& _ioContext;
  boost::asio::strand<boost::asio::io_context::executor_type>& _strand;
  boost::asio::ip::tcp::socket& _socket;
  AsioTimer _heartbeatTimer;
  unsigned& _heartbeatPeriod;
  char _heartbeatBuffer[HEADER_SIZE] = {};
};

} // end of namespace tcp
