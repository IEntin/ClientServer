/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "ThreadPool.h"
#include <boost/asio.hpp>

struct ClientOptions;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

using ConnectionDetailsPtr = std::shared_ptr<struct ConnectionDetails>;

class TcpClientHeartbeat final : public std::enable_shared_from_this<TcpClientHeartbeat>,
  public RunnableT<TcpClientHeartbeat> {

 public:

  TcpClientHeartbeat(const ClientOptions& options);

  ~TcpClientHeartbeat() override;

  bool start() override;

  void stop() override;

 private:

  void run() noexcept override;

  void heartbeatWait();

  void timeoutWait();

  void write();

  void read();

  const ClientOptions& _options;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _periodTimer;
  AsioTimer _timeoutTimer;
  char _heartbeatBuffer[HEADER_SIZE] = {};
  ThreadPool _threadPool;
};

} // end of namespace tcp
