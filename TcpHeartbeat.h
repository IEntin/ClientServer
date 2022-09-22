/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "ObjectCounter.h"
#include "Runnable.h"
#include <boost/asio.hpp>

struct ServerOptions;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

using SessionDetailsPtr = std::shared_ptr<struct SessionDetails>;

using TcpAcceptorPtr = std::shared_ptr<class TcpAcceptor>;

class TcpHeartbeat final : public std::enable_shared_from_this<TcpHeartbeat>, public Runnable {

 public:

  TcpHeartbeat(const ServerOptions& options, SessionDetailsPtr details, TcpAcceptorPtr parent);
  ~TcpHeartbeat() override;

  bool start() override;
  void stop() override {}

 private:

  void run() noexcept override;

  unsigned getNumberObjects() const override;

  void heartbeatWait();

  void heartbeat();
  const ServerOptions& _options;
  SessionDetailsPtr _details;
  TcpAcceptorPtr _parent;
  boost::asio::io_context& _ioContext;
  boost::asio::strand<boost::asio::io_context::executor_type> _strand;
  boost::asio::ip::tcp::socket& _socket;
  AsioTimer _heartbeatTimer;
  int _heartbeatPeriod;
  char _heartbeatBuffer[HEADER_SIZE] = {};
  ObjectCounter<TcpHeartbeat> _objectCounter;
};

} // end of namespace tcp
