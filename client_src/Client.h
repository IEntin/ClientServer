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

 public:

  Client(const ClientOptions& options);
  ~Client() = default;

  bool preparePackage(const Batch& payload, Batch& modified, size_t bufferSize);

  static std::string createRequestId(size_t index);

  static size_t createPayload(const char* sourceName, Batch& payload);

  static std::string readFileContent(const std::string& name);
};
