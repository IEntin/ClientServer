/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <boost/asio.hpp>

struct ServerOptions;

namespace tcp {

using SessionDetailsPtr = std::shared_ptr<struct SessionDetails>;

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class TcpHeartbeat : public std::enable_shared_from_this<TcpHeartbeat>, public Runnable {
public:
  TcpHeartbeat(const ServerOptions& options, SessionDetailsPtr details);
  ~TcpHeartbeat() override;

  bool start() override;

private:
  void run() noexcept override;
  void stop() override;

  void readToken();
  void asyncWait();
  void sendToken();
  const ServerOptions& _options;
  SessionDetailsPtr _details;
  boost::asio::io_context& _ioContext;
  boost::asio::ip::tcp::socket& _socket;
  AsioTimer _timer;
  char _buffer = '\0';
  RunnablePtr _parent;
};

} // end of namespace tcp
