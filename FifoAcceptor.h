/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include <map>

struct ServerOptions;

class Server;

class ThreadPoolSessions;

using SessionMap = std::map<std::string, RunnableWeakPtr>;

enum class HEADERTYPE : char;

namespace fifo {

class FifoAcceptor : public std::enable_shared_from_this<FifoAcceptor>,
  public RunnableT<FifoAcceptor> {
  void run() override;
  bool start() override;
  void stop() override;
  std::pair<HEADERTYPE, std::string> unblockAcceptor();
  RunnablePtr createSession();
  void removeFifoFiles();
  void destroySession(const std::string& key);
  const ServerOptions& _options;
  Server& _server;
  SessionMap _sessions;
  ThreadPool _threadPoolAcceptor;
  ThreadPoolSessions& _threadPoolSession;
 public:
  FifoAcceptor(const ServerOptions& options, Server& server);
  ~FifoAcceptor() override;
};

} // end of namespace fifo
