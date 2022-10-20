/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Runnable.h"
#include "ThreadPool.h"
#include <boost/asio.hpp>

struct ClientOptions;

namespace tcp {

using TcpClientHeartbeatPtr = std::shared_ptr<class TcpClientHeartbeat>;

class TcpClientHeartbeat : public std::enable_shared_from_this<TcpClientHeartbeat>, public Runnable {

  void run() noexcept override;

  const ClientOptions& _options;

  ThreadPool _threadPool;

  boost::asio::io_context _ioContext;

  boost::asio::ip::tcp::socket _socket;

  char _heartbeatBuffer[HEADER_SIZE] = {};

 public:

  TcpClientHeartbeat(const ClientOptions& options, std::string_view clientId);

  ~TcpClientHeartbeat() override;

  bool start() override;
  void stop() override;
};

} // end of namespace tcp
