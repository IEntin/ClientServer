/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>

struct ServerOptions;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

namespace tcp {
  class TcpServer;
  using TcpServerPtr = std::shared_ptr<TcpServer>;
}

namespace fifo {
  class FifoServer;
  using FifoServerPtr = std::shared_ptr<FifoServer>;
}

class ControllerStrategy {

  tcp::TcpServerPtr _tcpServer;

  fifo::FifoServerPtr _fifoServer;

 public:
  ControllerStrategy() = default;

  ~ControllerStrategy() = default;

  void onCreate(const ServerOptions& options);

  int onStart(const ServerOptions& options, TaskControllerPtr taskController);

  void onStop();

};
