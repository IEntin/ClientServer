/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include <map>

struct ServerOptions;

class SessionContainer;

using SessionMap = std::map<std::string, RunnableWeakPtr>;

enum class HEADERTYPE : char;

namespace fifo {

using FifoSessionWeakPtr = std::weak_ptr<class FifoSession>;

class FifoAcceptor : public std::enable_shared_from_this<FifoAcceptor>,
  public RunnableT<FifoAcceptor> {
  void run() override;
  std::pair<HEADERTYPE, std::string> unblockAcceptor();
  bool createSession();
  void removeFifoFiles();
  void destroySession(const std::string& key);
  const ServerOptions& _options;
  SessionContainer& _sessionContainer;
  SessionMap& _sessions;
  ThreadPool _threadPoolAcceptor;
  ThreadPool _threadPoolSession;
 public:
  FifoAcceptor(const ServerOptions& options, SessionContainer& sessionContainer);
  ~FifoAcceptor() override;
  bool start() override;
  void stop() override;
};

} // end of namespace fifo
