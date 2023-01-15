/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "StrategySelector.h"
#include <map>
#include <mutex>

using SessionMap = std::map<std::string, RunnableWeakPtr>;

struct ServerOptions;
class Strategy;

class Server {
 public:
  Server(const ServerOptions& options);
  ~Server() = default;
  SessionMap _fifoSessions;
  SessionMap _tcpSessions;
  bool start();
  void stop();
  STATUS incrementTotalSessions();
  STATUS decrementTotalSessions();
  std::atomic<unsigned>& totalSessions();
  // can be any SessionMap map
  const SessionMap::iterator _itEnd = _fifoSessions.end();
 private:
  const ServerOptions& _options;
  StrategySelector _strategySelector;
  Strategy& _strategy;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  std::atomic<STATUS> _status;
  std::atomic<unsigned> _totalSessions;
  std::mutex _mutex;
};
