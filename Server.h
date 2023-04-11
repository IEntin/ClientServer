/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolDiffObj.h"
#include <map>
#include <mutex>

struct ServerOptions;
using SessionMap = std::map<std::string, RunnableWeakPtr>;

class Server {
public:
  Server(const ServerOptions& options);
  ~Server();
  bool start();
  void stop();
  const ServerOptions& getOptions() const { return _options; }
  ThreadPoolBase& getThreadPoolAcceptor() { return _threadPoolAcceptor; }
  ThreadPoolDiffObj& getThreadPoolSession() { return _threadPoolSession; }
  bool startSession(std::string_view clientId, RunnablePtr session);
  void destroySession(const std::string& key);
  void stopSessions();
private:
  const ServerOptions& _options;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  ThreadPoolBase _threadPoolAcceptor;
  ThreadPoolDiffObj _threadPoolSession;
  SessionMap _sessions;
  std::mutex _mutex;
};
