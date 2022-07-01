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

using TaskControllerPtr = std::shared_ptr<class TaskController>;

class Strategy {
 public:
  Strategy() = default;

  virtual ~Strategy();

  virtual void onCreate(const ServerOptions& options) = 0;

   virtual int onStart(const ServerOptions& options, TaskControllerPtr taskController) = 0;

   virtual void onStop() = 0;

 protected:

  tcp::TcpServerPtr _tcpServer;

  fifo::FifoServerPtr _fifoServer;

};
