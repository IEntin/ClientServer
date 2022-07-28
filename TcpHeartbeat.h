/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <boost/asio.hpp>

struct ServerOptions;

class ThreadPool;

namespace tcp {

using AsioTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

class TcpHeartbeat : public std::enable_shared_from_this<TcpHeartbeat>, public Runnable {
public:
  TcpHeartbeat(const ServerOptions& options);
  ~TcpHeartbeat() override;

  void run() noexcept override;
  bool start() override;
  void stop() override;

  auto& endpoint() { return _endpoint; }
  auto& socket() { return _socket; }
private:
  void readToken();
  void write(std::string_view reply);
  void handleReadToken(const boost::system::error_code& ec, size_t transferred);
  void handleWriteReply(const boost::system::error_code& ec, size_t transferred);
  void asyncWait();
  bool sendToken();
  const ServerOptions& _options;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::socket _socket;
  AsioTimer _timer;
  char _buffer = '\0';
  RunnablePtr _parent;
};

} // end of namespace tcp
