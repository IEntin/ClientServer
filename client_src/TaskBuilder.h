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

enum class COMPRESSORS : char;

enum class TaskBuilderState {
  NONE,
  SUBTASKDONE,
  TASKDONE,
  ERROR
};

class TaskBuilder : public Runnable {

  bool compressSubtask(char* beg, char* end, bool alldone);

  int copyRequestWithId(char* dst, std::string_view line);

  std::ifstream _input;
  std::vector<char> _task;
  const std::string _sourceName;
  const COMPRESSORS _compressor;
  const bool _diagnostics;
  std::promise<void> _promise1;
  std::promise<void> _promise2;
  ssize_t _requestIndex;
  int _nextIdSz;
  TaskBuilderState _state = TaskBuilderState::NONE;
  void run() override;
  bool start() override;
  void stop() override;

 public:

  TaskBuilder(const struct ClientOptions& options);
  ~TaskBuilder() override;
  TaskBuilderState getTask(std::vector<char>& task);
  bool createTask();
};
