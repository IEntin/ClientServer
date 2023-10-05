/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolDiffObj.h"
#include <map>

using StrategyPtr = std::unique_ptr<class Strategy>;
using SessionMap = std::map<std::string, RunnableWeakPtr>;

class Server {
public:
  Server(StrategyPtr strategy);
  ~Server();
  bool start();
  void stop();
  bool startSession(RunnablePtr session);
  void stopSessions();
private:
  SessionMap _sessions;
  ThreadPoolBase _threadPoolAcceptor;
  ThreadPoolDiffObj _threadPoolSession;
  StrategyPtr _strategy;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  std::mutex _mutex;
};
