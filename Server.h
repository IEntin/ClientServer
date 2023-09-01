/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolDiffObj.h"
#include <map>

struct ServerOptions;
using StrategyPtr = std::shared_ptr<class Strategy>;
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
  StrategyPtr _strategy;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  ThreadPoolBase _threadPoolAcceptor;
  ThreadPoolDiffObj _threadPoolSession;
  SessionMap _sessions;
  std::mutex _mutex;
};
