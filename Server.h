/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <map>
#include <mutex>

using SessionMap = std::map<std::string, RunnableWeakPtr>;

struct ServerOptions;

class ThreadPool;

class Server {
public:
  Server(const ServerOptions& options);
  ~Server() = default;
  bool start();
  void stop();
  void registerSession(RunnablePtr weakPtr, ThreadPool& threadPool);
  void deregisterSession(RunnableWeakPtr weakPtr);
  static std::atomic<int>& totalSessions() { return _totalSessions; }
private:
  const ServerOptions& _options;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  using WaitingMap = std::map<int, RunnableWeakPtr>;
  WaitingMap _waitingSessions;
  std::atomic<int> _mapIndex = 0;
  std::mutex _mutex;
  void removeFromMap(RunnablePtr runnable);
  static inline std::atomic<int> _totalSessions = 0;
};
