/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "ThreadPool.h"
#include <atomic>

struct ClientOptions;

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;

class Client {

 protected:

  Client(const ClientOptions& options);

  bool processTask(TaskBuilderPtr taskBuilder);

  bool printReply(const std::vector<char>& buffer, const HEADER& header);

  const ClientOptions& _options;

  ThreadPool _threadPoolTaskBuilder;

  ThreadPool _threadPoolTcpHeartbeat;

  std::atomic<STATUS> _status = STATUS::NONE;

  static std::atomic_flag _stopFlag;

 public:

  virtual ~Client();

  virtual bool send(const std::vector<char>& msg) = 0;

  virtual bool receive() = 0;

  bool run();

  static void setStopFlag();

  static bool stopped();

};
