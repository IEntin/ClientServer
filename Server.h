/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolBase.h"
#include "ThreadPoolSession.h"

struct ServerOptions;

class Server {
public:
  Server(const ServerOptions& options);
  ~Server() = default;
  bool start();
  void stop();
private:
  const ServerOptions& _options;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  ThreadPoolBase _threadPoolAcceptor;
  ThreadPoolSession _threadPoolSession;
};
