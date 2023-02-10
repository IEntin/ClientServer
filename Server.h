/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPoolBase.h"
#include "ThreadPoolSession.h"

struct ServerOptions;

class Server {
public:
  Server(const ServerOptions& options);
  ~Server() = default;
  bool start();
  void stop();
  ThreadPoolBase& getThreadPoolAcceptor() { return _threadPoolAcceptor; }
  ThreadPoolSession& getThreadPoolSession() { return _threadPoolSession; }
  static std::atomic<int>& totalSessions() { return _totalSessions; }
private:
  const ServerOptions& _options;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  ThreadPoolBase _threadPoolAcceptor;
  ThreadPoolSession _threadPoolSession;
  static inline std::atomic<int> _totalSessions = 0;
};
