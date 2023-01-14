/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "SessionContainer.h"
#include <memory>

struct ServerOptions;

class Strategy {

 public:

  Strategy(const ServerOptions& options);

  virtual ~Strategy() {};

  virtual void create(const ServerOptions& options) = 0;

  bool start();

  void stop();

 protected:

  const ServerOptions& _options;

  RunnableWeakPtr _tcpAcceptor;

  RunnableWeakPtr _fifoAcceptor;

  SessionContainer _sessionContainer;

};
