/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <map>
#include <mutex>

using SessionMap = std::map<std::string, RunnableWeakPtr>;

struct ServerOptions;

class Server {
public:
  Server(const ServerOptions& options);
  ~Server() = default;
  bool start();
  void stop();
  unsigned registerSession(RunnableWeakPtr weakPtr);
  void deregisterSession();
  struct CountRunningSessions {
    CountRunningSessions() { _runningSessions++; }
    ~CountRunningSessions() { _runningSessions--; }
  };
private:
  const ServerOptions& _options;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  std::atomic<unsigned> _totalSessions;
  using WaitingMap = std::map<unsigned, RunnableWeakPtr>;
  WaitingMap _waitingSessions;
  std::atomic<unsigned> _mapIndex = 0;
  std::mutex _mutex;
  static std::atomic<unsigned> _runningSessions;
};
