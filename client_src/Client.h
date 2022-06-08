/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "MemoryPool.h"
#include "ThreadPool.h"

using Vectors = std::vector<std::vector<char>>;

struct ClientOptions;

class Client {

 protected:

  Client(const ClientOptions& options);

  bool processTask();

  bool printReply(const std::vector<char>& buffer, size_t uncomprSize, size_t comprSize, bool bcompressed);

  const ClientOptions& _options;

  Vectors _task;

  MemoryPool _memoryPool;

 private:

  ThreadPool _threadPool;

 public:

  virtual ~Client();

  virtual bool send(const std::vector<char>& subtask) = 0;

  virtual bool receive() = 0;

  virtual bool run();

  const Vectors& getTask() const { return _task; }
};
