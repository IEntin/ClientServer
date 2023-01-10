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
  static STATUS incrementTotalSessions();
  static STATUS decrementTotalSessions();
  static std::atomic<unsigned>& totalSessions();
  static const ServerOptions* _options;
  static SessionMap _sessions;
  static std::atomic<STATUS> _status;
  static std::atomic<unsigned> _totalSessions;
  static std::mutex _sessionMutex;
};
