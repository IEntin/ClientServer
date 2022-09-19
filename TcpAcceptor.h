/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include "Compression.h"
#include <boost/asio.hpp>
#include <map>

struct ServerOptions;

namespace tcp {

class TcpAcceptor : public std::enable_shared_from_this<TcpAcceptor>, public Runnable {
 public:
  TcpAcceptor(const ServerOptions& options);
  ~TcpAcceptor() override;

  void pushHeartbeat(RunnablePtr heartbeat);

  bool stopped() const { return _stopped; }

private:
  void accept();

  void run() override;

  bool start() override;

  void stop() override;

  const ServerOptions& _options;
  boost::asio::io_context _ioContext;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  ThreadPool _threadPool;
  ThreadPool _threadPoolHeartbeat;
  std::map<std::string, std::weak_ptr<class TcpSession>> _sessions;
  std::atomic<bool> _stopped = false;
};

} // end of namespace tcp
