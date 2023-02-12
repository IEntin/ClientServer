/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPoolReference.h"
#include <future>
#include <string_view>
#include <vector>
#include <fstream>

struct ClientOptions;

enum class TaskBuilderState : int {
  NONE,
  SUBTASKDONE,
  TASKDONE,
  ERROR,
  STOPPED
};

struct Subtask {
  std::vector<char> _chars;
  std::promise<void> _promise;
  TaskBuilderState _state = TaskBuilderState::NONE;
};

class TaskBuilder final : public RunnableT<TaskBuilder> {

  TaskBuilderState compressSubtask(Subtask& subtask, char* beg, char* end, bool alldone);
  int copyRequestWithId(char* dst, std::string_view line);

  const ClientOptions& _options;
  std::ifstream _input;
  std::vector<Subtask> _subtasks;
  std::atomic<int> _subtaskConsumeIndex = 0;
  std::atomic<unsigned> _subtaskProduceIndex = 0;
  ssize_t _requestIndex = 0;
  int _nextIdSz = 4;
  ThreadPoolReference _threadPool;
  void run() override;
  bool start() override;
 public:
  TaskBuilder(const ClientOptions& options, ThreadPoolBase& threadPool);
  ~TaskBuilder() override;
  void stop() override;
  TaskBuilderState getTask(std::vector<char>& task);
  TaskBuilderState createSubtask();
};
