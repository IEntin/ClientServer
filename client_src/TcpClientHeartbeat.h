/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ObjectCounter.h"
#include "Runnable.h"
#include "ThreadPool.h"
#include <boost/asio.hpp>

struct ClientOptions;

namespace tcp {

using TcpClientHeartbeatPtr = std::shared_ptr<class TcpClientHeartbeat>;

class TcpClientHeartbeat : public std::enable_shared_from_this<TcpClientHeartbeat>, public Runnable {

  void run() noexcept override;

  unsigned getNumberObjects() const override;

  bool receiveStatus();

  const ClientOptions& _options;

  boost::asio::io_context _ioContext;

  boost::asio::ip::tcp::socket _socket;

  ThreadPool _threadPool;

  ObjectCounter<TcpClientHeartbeat> _objectCounter;

  std::string _heartbeatId;

  TcpClientHeartbeatPtr _self;

 public:

  TcpClientHeartbeat(const ClientOptions& options);

  ~TcpClientHeartbeat() override;

  bool start() override;

  void stop() override;
};

} // end of namespace tcp
