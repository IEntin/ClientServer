#pragma once

#include "ThreadPool.h"
#include <future>

using Vectors = std::vector<std::vector<char>>;
using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
enum class COMPRESSORS : unsigned short;

class TaskBuilder : public Runnable {

  bool compressSubtasks();

  Vectors _task;
  const std::string _sourceName;
  const COMPRESSORS _compressor;
  const bool _diagnostics;
  bool _done = false;
  std::promise<void> _promise;

 public:

  TaskBuilder(const std::string& sourceName, COMPRESSORS compressor, bool diagnostics);
  ~TaskBuilder() override;
  void run() noexcept override;
  bool getTask(Vectors& task);
  bool createRequests();
};
