/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include <boost/asio.hpp>

struct ServerOptions;

namespace tcp {

class TcpHeartbeatAcceptor : public std::enable_shared_from_this<TcpHeartbeatAcceptor>, public Runnable {
public:
  TcpHeartbeatAcceptor(const ServerOptions& options);
  ~TcpHeartbeatAcceptor() override;
private:
  void run() override;

  bool start() override;

  void stop() override;

  void accept();

  const ServerOptions& _options;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  ThreadPool _threadPool;
};

} // end of namespace tcp
