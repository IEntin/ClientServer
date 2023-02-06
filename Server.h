/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPoolSessions.h"
#include <mutex>

struct ServerOptions;

class Server {
public:
  Server(const ServerOptions& options);
  ~Server() = default;
  bool start();
  void stop();
  ThreadPoolSessions& getThreadPool() { return _threadPoolSessions; }
  static std::atomic<int>& totalSessions() { return _totalSessions; }
private:
  const ServerOptions& _options;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  std::mutex _mutex;
  ThreadPoolSessions _threadPoolSessions;
  static inline std::atomic<int> _totalSessions = 0;
};
