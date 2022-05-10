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

  virtual bool processTask() = 0;

  Vectors _task;

  const ClientOptions& _options;

  ThreadPool _threadPool;

  MemoryPool _memoryPool;

  virtual bool run();

  bool printReply(const std::vector<char>& buffer, size_t uncomprSize, size_t comprSize, bool bcompressed);

 public:

  Client(const ClientOptions& options);
  virtual ~Client();

  const Vectors& getTask() const { return _task; }
};
