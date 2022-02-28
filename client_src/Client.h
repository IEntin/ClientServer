#pragma once

#include "ThreadPool.h"
#include <string>
#include <vector>

using Batch = std::vector<std::string>;

struct ClientOptions;

class Client {
 protected:

  virtual bool processTask() = 0;

  const ClientOptions& _options;

  Batch _task;

  ThreadPool _threadPool;

  bool loop();

 public:

  Client(const ClientOptions& options);
  ~Client();

  const Batch& getTask() const { return _task; }

  static std::string readFile(const std::string& name);
};
