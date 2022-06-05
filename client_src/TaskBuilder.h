/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <future>
#include <memory>
#include <string_view>
#include <vector>

using Vectors = std::vector<std::vector<char>>;
using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
enum class COMPRESSORS : short;
struct MemoryPool;

class TaskBuilder : public Runnable {

  bool compressSubtask(std::vector<char>&& subtask);

  int copyRequestWithId(char* dst, std::string_view line);

  Vectors _task;
  const std::string _sourceName;
  const COMPRESSORS _compressor;
  const bool _diagnostics;
  MemoryPool& _memoryPool;
  bool _done = false;
  std::promise<void> _promise;
  unsigned long long _requestIndex = 0;

 public:

  TaskBuilder(const struct ClientOptions& options, MemoryPool& memoryPool);
  ~TaskBuilder() override;
  void run() noexcept override;
  bool getTask(Vectors& task);
  bool createTask();
};
