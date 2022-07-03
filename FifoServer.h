/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include <memory>
#include <vector>

using Response = std::vector<std::string>;

struct ServerOptions;

namespace fifo {

using FifoServerPtr = std::shared_ptr<class FifoServer>;

using FifoConnectionPtr = std::shared_ptr<class FifoConnection>;

class FifoServer : public std::enable_shared_from_this<FifoServer>, public Runnable {
  const ServerOptions& _options;
  const std::string _fifoDirName;
  ThreadPool _threadPool;
  void run() override;
  void removeFifoFiles();
  std::vector<std::string> _fifoNames;
  void wakeupPipes();
 public:
  FifoServer(const ServerOptions& options);
  ~FifoServer();
  bool start(const ServerOptions& options);
  void stop();
};

} // end of namespace fifo
