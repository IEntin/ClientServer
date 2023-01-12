/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <map>
#include <mutex>

using SessionMap = std::map<std::string, RunnableWeakPtr>;

struct ServerOptions;

class SessionContainer {
 public:
  SessionContainer(const ServerOptions& options);
  ~SessionContainer() = default;
  SessionMap _fifoSessions;
  SessionMap _tcpSessions;
  // can be any SessionMap map
  const SessionMap::iterator _itEnd = _fifoSessions.end();
  STATUS incrementTotalSessions();
  STATUS decrementTotalSessions();
  std::atomic<unsigned>& totalSessions();
 private:
  const ServerOptions& _options;
  std::atomic<STATUS> _status;
  std::atomic<unsigned> _totalSessions;
  std::mutex _mutex;
};
