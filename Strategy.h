/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>

struct ServerOptions;

namespace tcp {
  class TcpServer;
  using TcpServerPtr = std::shared_ptr<TcpServer>;
}

namespace fifo {
  class FifoServer;
  using FifoServerPtr = std::shared_ptr<FifoServer>;
}

class Strategy {

 public:

  virtual ~Strategy();

  virtual void onCreate(const ServerOptions& options) = 0;

  virtual int onStart(const ServerOptions& options) = 0;

  virtual void onStop() = 0;

 protected:

  Strategy() = default;

  tcp::TcpServerPtr _tcpServer;

  fifo::FifoServerPtr _fifoServer;

};
