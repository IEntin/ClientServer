/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "ThreadPool.h"

struct ClientOptions;

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;

class Client {

 protected:

  Client(const ClientOptions& options);

  bool processTask(TaskBuilderPtr taskBuilder);

  bool printReply(const std::vector<char>& buffer, const HEADER& header);

  const ClientOptions& _options;

  ThreadPool _threadPool;

 public:

  virtual ~Client();

  virtual bool send(const std::vector<char>& msg) = 0;

  virtual bool receive() = 0;

  virtual bool run();

};
