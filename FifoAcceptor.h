/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolReference.h"

class Server;
struct ServerOptions;

namespace fifo {

class FifoAcceptor : public std::enable_shared_from_this<FifoAcceptor>,
  public RunnableT<FifoAcceptor> {
  void run() override;
  bool start() override;
  void stop() override;
  std::pair<HEADERTYPE, std::string> unblockAcceptor();
  void createSession();
  void removeFifoFiles();
  Server& _server;
  const ServerOptions& _options;
  ThreadPoolReference _threadPoolAcceptor;
 public:
  FifoAcceptor(Server& server);
  ~FifoAcceptor() override;
};

} // end of namespace fifo
