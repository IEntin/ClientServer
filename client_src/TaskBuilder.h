/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolReference.h"
#include <deque>
#include <mutex>
#include <string_view>
#include <vector>
#include <fstream>

struct ClientOptions;
class ThreadPoolBase;

enum class TaskBuilderState : int {
  NONE,
  SUBTASKDONE,
  TASKDONE,
  ERROR
};

struct Subtask {
  Subtask() = default;
  Subtask(const Subtask&) : _state(TaskBuilderState::NONE) {}
  ~Subtask() = default;
  HEADER _header;
  std::vector<char> _body;
  std::atomic<TaskBuilderState> _state = TaskBuilderState::NONE;
};

class TaskBuilder final : public RunnableT<TaskBuilder> {

  TaskBuilderState compressSubtask(Subtask& subtask,
				   const std::vector<char>& aggregate,
				   size_t aggregateSize,
				   bool alldone);
  int copyRequestWithId(char* dst, std::string_view line);

  const ClientOptions& _options;
  std::ifstream _input;
  std::deque<Subtask> _subtasks;
  ssize_t _requestIndex = 0;
  int _nextIdSz = 4;
  ThreadPoolReference<ThreadPoolBase> _threadPool;
  std::mutex _mutex;
  void run() override;
  bool start() override { return true; }
 public:
  TaskBuilder(const ClientOptions& options, ThreadPoolBase& threadPool);
  ~TaskBuilder() override;
  void stop() override {}
  TaskBuilderState getSubtask(Subtask& task);
  TaskBuilderState createSubtask();
};
