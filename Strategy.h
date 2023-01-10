/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "SessionContainer.h"
#include <memory>

struct ServerOptions;

namespace tcp {
  class TcpAcceptor;
}

using TcpAcceptorWeakPtr = std::weak_ptr<tcp::TcpAcceptor>;

namespace fifo {
  class FifoAcceptor;
}

using FifoAcceptorWeakPtr = std::weak_ptr<fifo::FifoAcceptor>;

class Strategy {

 public:

  Strategy(const ServerOptions& options);

  virtual ~Strategy() {};

  virtual void create(const ServerOptions& options) = 0;

  bool start();

  void stop();

 protected:

  const ServerOptions& _options;

  TcpAcceptorWeakPtr _tcpAcceptor;

  FifoAcceptorWeakPtr _fifoAcceptor;

  SessionContainer _sessionContainer;

};
