/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include "Compression.h"
#include <boost/asio.hpp>
#include <memory>

using TaskControllerPtr = std::shared_ptr<class TaskController>;

struct ServerOptions;

using RunnablePtr = std::shared_ptr<Runnable>;

namespace tcp {

using TcpServerPtr = std::shared_ptr<class TcpServer>;

using TcpConnectionPtr = std::shared_ptr<class TcpConnection>;

class TcpServer : public std::enable_shared_from_this<TcpServer>, public Runnable {
public:
  TcpServer(const ServerOptions& options, TaskControllerPtr taskController);
  ~TcpServer() override;
  bool start();
  void stop();
private:
  void accept();

  void handleAccept(TcpConnectionPtr connection,
		    const boost::system::error_code& ec);
  void run() override;

  const ServerOptions& _options;
  TaskControllerPtr _taskController;
  std::atomic<int>& _numberConnections;
  std::atomic<int> _numberTcpConnections = 0;
  boost::asio::io_context _ioContext;
  int _tcpPort;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  std::atomic<bool> _stopped = false;
  ThreadPool _threadPool;
};

} // end of namespace tcp
