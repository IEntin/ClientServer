/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include <vector>

struct ServerOptions;

using RunnableWeakPtr = std::weak_ptr<Runnable>;

namespace fifo {

class FifoAcceptor : public std::enable_shared_from_this<FifoAcceptor>, public Runnable {
  void run() override;
  bool start() override;
  void stop() override;
  unsigned getNumberObjects() const override;
  bool unblockAcceptor();
  void removeFifoFiles();
  const ServerOptions& _options;
  ThreadPool _threadPoolAcceptor;
  ThreadPool _threadPoolSession;
  std::vector<RunnableWeakPtr> _sessions;
  int _fd = -1;
  ObjectCounter<FifoAcceptor> _objectCounter;
 public:
  FifoAcceptor(const ServerOptions& options);
  ~FifoAcceptor() override;

  void remove(RunnablePtr toRemove);
};

} // end of namespace fifo
