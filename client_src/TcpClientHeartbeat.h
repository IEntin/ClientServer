/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolReference.h"
#include <boost/asio.hpp>

struct ClientOptions;
class ThreadPoolBase;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;
using ConnectionDetailsPtr = std::shared_ptr<struct ConnectionDetails>;

class TcpClientHeartbeat final : public std::enable_shared_from_this<TcpClientHeartbeat>,
  public RunnableT<TcpClientHeartbeat> {
 public:
  TcpClientHeartbeat(const ClientOptions& options, ThreadPoolBase& threadPoolClient);
  ~TcpClientHeartbeat() override;
 private:
  void run() override;
  bool start() override;
  void stop() override;
  void heartbeatWait();
  void timeoutWait();
  void write();
  void read();

  const ClientOptions& _options;
  ThreadPoolReference<ThreadPoolBase> _threadPoolClient;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _periodTimer;
  AsioTimer _timeoutTimer;
  char _heartbeatBuffer[HEADER_SIZE] = {};
  unsigned _period = 0;
  unsigned _timeout = 0;
};

} // end of namespace tcp
