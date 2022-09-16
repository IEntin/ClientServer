/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include "Compression.h"
#include <boost/asio.hpp>
#include <map>
#include <memory>

struct ServerOptions;

namespace tcp {

class TcpServer : public std::enable_shared_from_this<TcpServer>, public Runnable {
public:
  TcpServer(const ServerOptions& options);
  ~TcpServer() override;

  void pushHeartbeat(RunnablePtr heartbeat);

private:
  void accept();

  void run() override;

  bool start() override;

  void stop() override;

  const ServerOptions& _options;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  ThreadPool _threadPoolHeartbeat;
  ThreadPool _threadPool;
  std::map<std::string, std::weak_ptr<class TcpSession>> _sessions;
};

} // end of namespace tcp
