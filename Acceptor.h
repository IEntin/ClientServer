/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <map>

struct ServerOptions;

using SessionMap = std::map<std::string, RunnableWeakPtr>;

class Server;

class ThreadPoolSessions;

class Acceptor : public RunnableT<Acceptor> {
public:
  ~Acceptor() override;
protected:
  Acceptor(const ServerOptions& options, Server& server);
  void stop() override = 0;
  bool startSession(std::string_view clientId, RunnablePtr session);
  void destroySession(const std::string& key);

  const ServerOptions& _options;
  Server& _server;
  ThreadPoolSessions& _threadPoolSession;
  SessionMap _sessions;
};
