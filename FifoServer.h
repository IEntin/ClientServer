/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include <memory>
#include <vector>

using Response = std::vector<std::string>;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

struct ServerOptions;

namespace fifo {

using FifoServerPtr = std::shared_ptr<class FifoServer>;

using FifoConnectionPtr = std::shared_ptr<class FifoConnection>;

class FifoServer : public std::enable_shared_from_this<FifoServer> {
  const ServerOptions& _options;
  TaskControllerPtr _taskController;
  const std::string _fifoDirName;
  ThreadPool _threadPool;
  std::atomic<int>& _numberConnections;
  std::atomic<int> _numberFifoConnections;
  std::atomic<bool> _stopped = false;
  void removeFifoFiles();
  std::vector<std::string> _fifoNames;
  void wakeupPipes();
 public:
  FifoServer(const ServerOptions& options, TaskControllerPtr taskController);
  ~FifoServer();
  bool start(const ServerOptions& options);
  void stop();
  bool stopped() const { return _stopped; }
};

} // end of namespace fifo
