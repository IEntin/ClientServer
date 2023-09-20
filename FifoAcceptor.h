/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"

class Server;

namespace fifo {

class FifoAcceptor : public Runnable {
  void run() override;
  bool start() override;
  void stop() override;
  HEADERTYPE unblockAcceptor();
  void removeFifoFiles();
  Server& _server;
 public:
  FifoAcceptor(Server& server);
  ~FifoAcceptor() override;
};

} // end of namespace fifo
