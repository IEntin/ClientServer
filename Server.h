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
  Server(const ServerOptions& options, StrategyPtr strategy);
  ~Server();
  bool start();
  void stop();
  const ServerOptions& getOptions() const { return _options; }
  bool startSession(RunnablePtr session);
  void stopSessions();
private:
  const ServerOptions& _options;
  StrategyPtr _strategy;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  ThreadPoolBase _threadPoolAcceptor;
  ThreadPoolDiffObj _threadPoolSession;
  SessionMap _sessions;
  std::mutex _mutex;
};
