/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Runnable.h"
#include <boost/asio.hpp>
#include <thread>

struct ClientOptions;

namespace tcp {

using TcpClientHeartbeatPtr = std::shared_ptr<class TcpClientHeartbeat>;

class TcpClientHeartbeat : public std::enable_shared_from_this<TcpClientHeartbeat>, public Runnable {

  void stop() override {}
  void run() noexcept override;

  const ClientOptions& _options;

  const std::string _clientId;

  std::jthread _thread;

  boost::asio::io_context _ioContext;

  boost::asio::ip::tcp::socket _socket;

  char _heartbeatBuffer[HEADER_SIZE] = {};

  static TcpClientHeartbeatPtr _heartbeat;

 public:

  TcpClientHeartbeat(const ClientOptions& options, std::string_view clientId);

  ~TcpClientHeartbeat() override;

  bool start() override;

  static void create(const ClientOptions& options, std::string_view clientId);
};

} // end of namespace tcp
