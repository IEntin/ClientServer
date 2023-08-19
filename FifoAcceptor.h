/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"

class Server;
struct ServerOptions;

namespace fifo {

class FifoAcceptor : public RunnableT<FifoAcceptor> {
  void run() override;
  bool start() override;
  void stop() override;
  HEADERTYPE unblockAcceptor();
  void removeFifoFiles();
  Server& _server;
  const ServerOptions& _options;
 public:
  FifoAcceptor(Server& server);
  ~FifoAcceptor() override;
};

} // end of namespace fifo
