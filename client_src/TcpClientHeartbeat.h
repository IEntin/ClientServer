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

class TcpClientHeartbeat : public std::enable_shared_from_this<TcpClientHeartbeat>, public Runnable {

  void run() noexcept override;

  const ClientOptions& _options;

  const std::string _clientId;

  std::jthread _thread;

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
