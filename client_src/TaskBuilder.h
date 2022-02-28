#pragma once

#include "ThreadPool.h"
#include <future>

using Batch = std::vector<std::string>;
using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
enum class COMPRESSORS : unsigned short;

class TaskBuilder : public std::enable_shared_from_this<TaskBuilder>, public Runnable {

  bool createRequestBatch(Batch& batch);
  bool mergePayload(const Batch& batch, Batch& aggregatedBatch, size_t bufferSize);
  bool buildMessage(const Batch& payload, Batch& message);
  static std::string createRequestId(size_t index);

  Batch _task;
  const std::string _sourceName;
  Batch _source;
  const COMPRESSORS _compressor;
  const bool _diagnostics;
  bool _done = false;
  std::promise<void> _promise;

 public:

  TaskBuilder(const std::string& sourceName, COMPRESSORS compressor, bool diagnostics);
  ~TaskBuilder() override;
  void run() noexcept override;
  bool isDone() const { return _done; }
  void getTask(Batch& task);
  bool buildTask(const Batch& payload);
};
