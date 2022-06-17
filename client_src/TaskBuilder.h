/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <future>
#include <memory>
#include <string_view>
#include <vector>
#include <fstream>

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
enum class COMPRESSORS : int;
struct MemoryPool;

enum class TaskBuilderState {
  NONE,
  SUBTASKDONE,
  TASKDONE,
  ERROR
};

class TaskBuilder : public Runnable {

  bool compressSubtask(char* beg, char* end);

  int copyRequestWithId(char* dst, std::string_view line);

  std::ifstream _input;
  std::vector<char> _task;
  const std::string _sourceName;
  const COMPRESSORS _compressor;
  const bool _diagnostics;
  MemoryPool& _memoryPool;
  std::promise<void> _promise;
  ssize_t _sourcePos;
  ssize_t _sourceSize;
  ssize_t _requestIndex;
  int _nextIdSz;
  TaskBuilderState _state = TaskBuilderState::NONE;

 public:

  TaskBuilder(const struct ClientOptions& options, MemoryPool& memoryPool, ssize_t sourceSize);
  ~TaskBuilder() override;
  void run() noexcept override;
  TaskBuilderState getTask(std::vector<char>& task);
  bool createTask();
};
