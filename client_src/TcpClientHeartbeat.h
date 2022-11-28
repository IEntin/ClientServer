/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include <boost/asio.hpp>

struct ClientOptions;

namespace tcp {

using TcpClientHeartbeatPtr = std::shared_ptr<class TcpClientHeartbeat>;

class TcpClientHeartbeat : public std::enable_shared_from_this<TcpClientHeartbeat>,
  public RunnableT<TcpClientHeartbeat> {

  void run() noexcept override;

  void stop() override;

  bool receiveStatus();

  const ClientOptions& _options;

  std::string _heartbeatId;

  boost::asio::io_context _ioContext;

  boost::asio::ip::tcp::socket _socket;

  ThreadPool _threadPool;

 public:

  TcpClientHeartbeat(const ClientOptions& options);

  ~TcpClientHeartbeat() override;

  bool start() override;

  bool destroy();
};

} // end of namespace tcp
