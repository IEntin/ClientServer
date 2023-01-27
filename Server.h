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
  static std::atomic<unsigned>& totalSessions() { return _totalSessions; }
private:
  const ServerOptions& _options;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  using WaitingMap = std::map<unsigned, RunnableWeakPtr>;
  WaitingMap _waitingSessions;
  std::atomic<unsigned> _mapIndex = 0;
  std::mutex _mutex;
  void removeFromMap(RunnablePtr runnable);
  static inline std::atomic<unsigned> _totalSessions = 0;
};
