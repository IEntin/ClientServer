/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include "Compression.h"
#include <boost/asio.hpp>
#include <memory>

struct ServerOptions;

namespace tcp {

class TcpServer : public std::enable_shared_from_this<TcpServer>, public Runnable {
public:
  TcpServer(const ServerOptions& options);
  ~TcpServer() override;
private:
  void accept();

  void run() override;

  bool start() override;

  void stop() override;

  const ServerOptions& _options;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  RunnablePtr _heartbeatAcceptor;
  ThreadPool _threadPool;
};

} // end of namespace tcp
