/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <future>
#include <string_view>
#include <vector>
#include <fstream>

struct ClientOptions;

enum class TaskBuilderState : int {
  NONE,
  SUBTASKDONE,
  TASKDONE,
  ERROR
};

struct Subtask {
  std::vector<char> _chars;
  std::promise<void> _promise;
  TaskBuilderState _state = TaskBuilderState::NONE;
};

class TaskBuilder final : public Runnable {

  TaskBuilderState compressSubtask(Subtask& subtask, char* beg, char* end, bool alldone);

  int copyRequestWithId(char* dst, std::string_view line);

  const ClientOptions& _options;
  std::ifstream _input;
  std::vector<Subtask> _subtasks;
  std::atomic<unsigned> _subtaskConsumeIndex = 0;
  std::atomic<unsigned> _subtaskProduceIndex = 0;
  ssize_t _requestIndex = 0;
  int _nextIdSz = 4;
  void run() override;
  bool start() override;
  void stop() override;

 public:

  TaskBuilder(const ClientOptions& options);
  ~TaskBuilder() override;
  unsigned getNumberObjects() const override;
  TaskBuilderState getTask(std::vector<char>& task);
  TaskBuilderState createSubtask();
  ObjectCounter<TaskBuilder> _objectCounter;
};
