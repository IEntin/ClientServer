/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPoolReference.h"
#include <map>
#include <mutex>

struct ServerOptions;
using SessionMap = std::map<std::string, RunnableWeakPtr>;
class ThreadPoolDiffObj;

class Acceptor : public RunnableT<Acceptor> {
public:
  ~Acceptor() override;
protected:
  Acceptor(const ServerOptions& options,
	   ThreadPoolBase& threadPoolAcceptor,
	   ThreadPoolDiffObj& threadPoolSession);
  void stop() override = 0;
  bool startSession(std::string_view clientId, RunnablePtr session);
  void destroySession(const std::string& key);

  const ServerOptions& _options;
  ThreadPoolReference _threadPoolAcceptor;
  ThreadPoolDiffObj& _threadPoolSession;
  inline static SessionMap _sessions;
  inline static std::mutex _mutex;
};
