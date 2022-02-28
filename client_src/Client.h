#pragma once

#include "ThreadPool.h"
#include <string>
#include <vector>

using Batch = std::vector<std::string>;

struct ClientOptions;

class Client {
 protected:

  const ClientOptions& _options;

  Batch _task;

  ThreadPool _threadPool;

 public:

  Client(const ClientOptions& options);
  ~Client();

  const Batch& getTask() const { return _task; }

  static std::string readFileContent(const std::string& name);
};
