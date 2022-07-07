/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"

struct ClientOptions;

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;

class Client {

 protected:

  Client(const ClientOptions& options);

  bool processTask(TaskBuilderPtr&& taskBuilder);

  bool printReply(const std::vector<char>& buffer, size_t uncomprSize, size_t comprSize, bool bcompressed);

  const ClientOptions& _options;

 private:

  ThreadPool _threadPool;

  TaskBuilderPtr _taskBuilder;

 public:

  virtual ~Client();

  virtual bool send(const std::string& fifoName, const std::vector<char>& msg) = 0;

  virtual bool receive() = 0;

  virtual bool requestConnection() = 0;

  virtual bool run();

};
