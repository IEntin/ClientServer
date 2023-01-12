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
  const ServerOptions& _options;
  std::mutex _sessionMutex;
  std::atomic<STATUS> _status;
  SessionMap _sessions;
  const SessionMap::iterator _itEnd = _sessions.end();
  STATUS incrementTotalSessions();
  STATUS decrementTotalSessions();
  std::atomic<unsigned>& totalSessions();
  std::atomic<unsigned> _totalSessions;
};
