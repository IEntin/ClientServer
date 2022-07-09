/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include <memory>

struct ServerOptions;

namespace fifo {

using FifoServerPtr = std::shared_ptr<class FifoServer>;

class FifoServer : public std::enable_shared_from_this<FifoServer>, public Runnable {
  const ServerOptions& _options;
  ThreadPool _threadPool;
  void run() override;
  RunnablePtr _acceptor;
  void removeFifoFiles();
 public:
  FifoServer(const ServerOptions& options);
  ~FifoServer() override;
  bool start() override;
  void stop() override;
};

} // end of namespace fifo
