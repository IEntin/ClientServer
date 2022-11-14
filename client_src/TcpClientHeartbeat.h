/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ObjectCounter.h"
#include "Runnable.h"
#include <boost/asio.hpp>

struct ClientOptions;

namespace tcp {

using TcpClientHeartbeatPtr = std::shared_ptr<class TcpClientHeartbeat>;

class TcpClientHeartbeat : public std::enable_shared_from_this<TcpClientHeartbeat>, public Runnable {

  void run() noexcept override;

  unsigned getNumberObjects() const override;

  bool closeHeartbeat();

  bool closeSession();

  void readStatus();

  const ClientOptions& _options;

  std::string _clientId;

  const std::string _sessionClientId;

  boost::asio::io_context _ioContext;

  boost::asio::ip::tcp::socket _socket;

  ObjectCounter<TcpClientHeartbeat> _objectCounter;

 public:

  TcpClientHeartbeat(const ClientOptions& options, std::string_view clientId);

  ~TcpClientHeartbeat() override;

  bool start() override;
  void stop() override;
};

} // end of namespace tcp
