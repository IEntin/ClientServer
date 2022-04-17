#pragma once

#include "ClientOptions.h"
#include "Runnable.h"
#include <future>
#include <vector>

using Vectors = std::vector<std::vector<char>>;
using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
enum class COMPRESSORS : unsigned short;
struct MemoryPool;

class TaskBuilder : public Runnable {

  bool compressSubtasks();

  Vectors _task;
  const std::string _sourceName;
  const COMPRESSORS _compressor;
  const bool _diagnostics;
  MemoryPool& _memoryPool;
  bool _done = false;
  std::promise<void> _promise;

 public:

  TaskBuilder(const ClientOptions& options, MemoryPool& memoryPool);
  ~TaskBuilder() override;
  void run() noexcept override;
  bool getTask(Vectors& task);
  bool createRequests();
};
