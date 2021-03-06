/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include <vector>

struct ServerOptions;

namespace fifo {

class FifoAcceptor : public std::enable_shared_from_this<FifoAcceptor>, public Runnable {
  bool sendStatusToClient(PROBLEMS problem);
  void removeFifoFiles();
  const ServerOptions& _options;
  ThreadPool _threadPool;
  std::string _acceptorName;
  std::vector<RunnableWeakPtr> _sessions;
  int _fd = -1;
  unsigned short _ephemeralIndex = 0;
 public:
  FifoAcceptor(const ServerOptions& options);
  ~FifoAcceptor() = default;
  void run() override;
  bool start() override;
  void stop() override;
};

} // end of namespace fifo
