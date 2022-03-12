#pragma once

#include "ThreadPool.h"
#include <string>
#include <vector>

using Vectors = std::vector<std::vector<char>>;

struct ClientOptions;

class Client {
 protected:

  virtual bool processTask() = 0;

  Vectors _task;

  ThreadPool _threadPool;

  bool loop(const ClientOptions& options);

  bool printReply(const ClientOptions& options,
		  const std::vector<char>& buffer,
		  size_t uncomprSize,
		  size_t comprSize,
		  bool bcompressed);

 public:

  Client(size_t bufferSize);
  ~Client();

  const Vectors& getTask() const { return _task; }

  static std::string readFile(const std::string& name);
};
