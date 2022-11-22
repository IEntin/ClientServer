/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

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

  virtual ~Strategy();

  virtual void create(const ServerOptions& options) = 0;

  virtual bool start(const ServerOptions& options) = 0;

  virtual void stop() = 0;

 protected:

  Strategy() = default;

  TcpAcceptorWeakPtr _tcpAcceptor;

  FifoAcceptorWeakPtr _fifoAcceptor;

};
