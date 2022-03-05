#pragma once

#include "ThreadPool.h"
#include <string>
#include <vector>

using Vectors = std::vector<std::vector<char>>;

struct ClientOptions;

class Client {
 protected:

  virtual bool processTask() = 0;

  const ClientOptions& _options;

  Vectors _task;

  ThreadPool _threadPool;

  bool loop();

 public:

  Client(const ClientOptions& options);
  ~Client();

  const Vectors& getTask() const { return _task; }

  static std::string readFile(const std::string& name);
};
