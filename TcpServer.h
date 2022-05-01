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

namespace tcp {

using RunnablePtr = std::shared_ptr<Runnable>;

using TcpServerPtr = std::shared_ptr<class TcpServer>;

using TcpConnectionPtr = std::shared_ptr<class TcpConnection>;

class TcpServer : public std::enable_shared_from_this<TcpServer>, public Runnable {
public:
  TcpServer(TaskControllerPtr taskController, const ServerOptions& options);
  ~TcpServer() override;
  bool start();
  void stop();
  bool stopped() const { return _stopped; }
  void pushToThreadPool(RunnablePtr runnable);
private:
  void accept();

  void handleAccept(TcpConnectionPtr connection,
		    const boost::system::error_code& ec);
  void run() noexcept override;

  TaskControllerPtr _taskController;
  const size_t _numberConnections;
  boost::asio::io_context _ioContext;
  unsigned _tcpPort;
  unsigned _timeout;
  COMPRESSORS _compressor;
  boost::asio::ip::tcp::endpoint _endpoint;
  boost::asio::ip::tcp::acceptor _acceptor;
  std::atomic<bool> _stopped = false;
  ThreadPool _threadPool;
};

} // end of namespace tcp
