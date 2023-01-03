/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include <map>

struct ServerOptions;

enum class HEADERTYPE : char;

namespace fifo {

using FifoSessionWeakPtr = std::weak_ptr<class FifoSession>;

class FifoAcceptor : public std::enable_shared_from_this<FifoAcceptor>,
  public RunnableT<FifoAcceptor> {
  void run() override;
  std::pair<HEADERTYPE, std::string> unblockAcceptor();
  bool createSession();
  void removeFifoFiles();
  void filterSessions();
  const ServerOptions& _options;
  ThreadPool _threadPoolAcceptor;
  ThreadPool _threadPoolSession;
  std::map<std::string, FifoSessionWeakPtr> _sessions;
 public:
  FifoAcceptor(const ServerOptions& options);
  ~FifoAcceptor() override;
  bool start() override;
  void stop() override;
};

} // end of namespace fifo
