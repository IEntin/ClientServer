/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <vector>

struct ServerOptions;

class ThreadPool;

namespace fifo {

using FifoServerPtr = std::shared_ptr<class FifoServer>;

class FifoAcceptor : public std::enable_shared_from_this<FifoAcceptor>, public Runnable {
  void wakeupPipe();
  void removeFifoFiles();
  const ServerOptions& _options;
  FifoServerPtr _server;
  ThreadPool& _threadPool;
  std::string _acceptorName;
  std::vector<RunnableWeakPtr> _connections;
  int _fd = -1;
  unsigned short _ephemeralIndex = 0;
 public:
  FifoAcceptor(const ServerOptions& options, FifoServerPtr server, ThreadPool& threadPool);
  ~FifoAcceptor() = default;
  void run() override;
  bool start() override;
  void stop() override;
};

} // end of namespace fifo
