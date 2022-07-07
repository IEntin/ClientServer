/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include <memory>
#include <string>
#include <vector>

using Response = std::vector<std::string>;

struct ServerOptions;

namespace fifo {

class FifoAcceptor : public std::enable_shared_from_this<FifoAcceptor>, public Runnable {
  void wakeupPipe();
  const ServerOptions& _options;
  RunnablePtr _server;
  std::string _acceptorName;
  int _fd = -1;
 public:
  FifoAcceptor(const ServerOptions& options, RunnablePtr server);
  ~FifoAcceptor() = default;
  void run() override;
  bool start() override;
  void stop() override;
};

class FifoServer : public std::enable_shared_from_this<FifoServer>, public Runnable {
  const ServerOptions& _options;
  const std::string _fifoDirName;
  ThreadPool _threadPool;
  void run() override;
  void removeFifoFiles();
  RunnablePtr _acceptor;
  std::vector<std::string> _fifoNames;
  void wakeupPipes();
 public:
  FifoServer(const ServerOptions& options);
  ~FifoServer() override;
  bool start() override;
  void stop() override;
};

} // end of namespace fifo
