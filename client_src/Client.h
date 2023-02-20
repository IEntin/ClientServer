/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolBase.h"
#include <atomic>

struct Subtask;
struct ClientOptions;
using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;

class Client {

 protected:
  Client(const ClientOptions& options);

  bool processTask(TaskBuilderPtr taskBuilder);
  bool printReply(const std::vector<char>& buffer, const HEADER& header);
  void start();

  const ClientOptions& _options;

  ThreadPoolBase _threadPoolClient;
  std::atomic<STATUS> _status = STATUS::NONE;
  std::string _clientId;
  RunnablePtr _heartbeat;
  TaskBuilderPtr _taskBuilder;
  static std::atomic_flag _stopFlag;

 public:
  virtual ~Client();

  virtual bool send(const Subtask& subtask) = 0;
  virtual bool receive() = 0;
  virtual bool receiveStatus() = 0;
  virtual bool run() = 0;
  virtual bool destroySession() = 0;

  void stop();
};
