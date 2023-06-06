/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolBase.h"

struct Subtask;
struct ClientOptions;
using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;

class Client {

 protected:
  Client(const ClientOptions& options);

  bool processTask(TaskBuilderPtr taskBuilder);
  bool printReply(std::string_view buffer, const HEADER& header);
  void start();

  const ClientOptions& _options;

  ThreadPoolBase _threadPoolClient;
  std::atomic<STATUS> _status = STATUS::NONE;
  RunnablePtr _heartbeat;
  TaskBuilderPtr _taskBuilder;

 public:
  virtual ~Client();

  virtual bool send(const Subtask& subtask) = 0;
  virtual bool receive() = 0;
  virtual bool receiveStatus() = 0;
  virtual bool run() = 0;

  void stop();
};
