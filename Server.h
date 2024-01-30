/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <map>

#include "Chronometer.h"
#include "ThreadPoolDiffObj.h"

using StrategyPtr = std::unique_ptr<class Strategy>;
using SessionMap = std::map<std::size_t, RunnableWeakPtr>;

class Server {
public:
  Server(StrategyPtr strategy);
  ~Server();
  bool start();
  void stop();
  bool startSession(RunnablePtr session);
  bool removeFromSessions(std::size_t clientId);
  void stopSessions();
private:
  Chronometer _chronometer;
  SessionMap _sessions;
  ThreadPoolBase _threadPoolAcceptor;
  ThreadPoolDiffObj _threadPoolSession;
  StrategyPtr _strategy;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  std::mutex _mutex;
};
