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
  SessionMap _fifoSessions;
  SessionMap _tcpSessions;
  bool start();
  void stop();
  std::pair<STATUS, unsigned> incrementTotalSessions();
  STATUS decrementTotalSessions();
  // can be any SessionMap map
  const SessionMap::iterator _itEnd = _fifoSessions.end();
 private:
  const ServerOptions& _options;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  std::atomic<STATUS> _status;
  std::atomic<unsigned> _totalSessions;
  std::mutex _mutex;
};
