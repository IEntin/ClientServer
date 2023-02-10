/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <map>

struct ServerOptions;
using SessionMap = std::map<std::string, RunnableWeakPtr>;
class ThreadPoolBase;
class ThreadPoolSession;

class Acceptor : public RunnableT<Acceptor> {
public:
  ~Acceptor() override;
protected:
  Acceptor(const ServerOptions& options,
	   ThreadPoolBase& threadPoolAcceptor,
	   ThreadPoolSession& threadPoolSession);
  void stop() override = 0;
  bool startSession(std::string_view clientId, RunnablePtr session);
  void destroySession(const std::string& key);

  const ServerOptions& _options;
  ThreadPoolBase& _threadPoolAcceptor;
  ThreadPoolSession& _threadPoolSession;
  SessionMap _sessions;
};
