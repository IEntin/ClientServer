#pragma once

#include <string>
#include <vector>

using Batch = std::vector<std::string>;

struct ClientOptions;

class Client {
 protected:

  bool mergePayload(const Batch& batch, Batch& aggregatedBatch, size_t bufferSize);

  bool buildMessage(const Batch& payload, Batch& message);

  const ClientOptions& _options;

  static std::string createRequestId(size_t index);

  Batch _task;
 public:

  Client(const ClientOptions& options);
  ~Client() = default;

  static size_t createPayload(const char* sourceName, Batch& payload);

  bool buildTask(const Batch& payload, size_t bufferSize);

  const Batch& getTask() const { return _task; }

  static std::string readFileContent(const std::string& name);
};
