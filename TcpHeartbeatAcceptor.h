/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include <boost/asio.hpp>

struct ServerOptions;

namespace tcp {

using TcpHeartbeatPtr = std::shared_ptr<class TcpHeartbeat>;

class TcpHeartbeatAcceptor : public std::enable_shared_from_this<TcpHeartbeatAcceptor>, public Runnable {
public:
  TcpHeartbeatAcceptor(const ServerOptions& options);
  ~TcpHeartbeatAcceptor() override;
private:
  void run() override;

  bool start() override;

  void stop() override;

  void accept();

  void handleAccept(TcpHeartbeatPtr heartbeat, const boost::system::error_code& ec);

  const ServerOptions& _options;
  boost::asio::io_context _ioContext;
  unsigned short _tcpHeartbeatPort;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  RunnablePtr _heartbeatAcceptor;
  ThreadPool _threadPool;
};

} // end of namespace tcp
