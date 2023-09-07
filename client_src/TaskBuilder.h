/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "Subtask.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <fstream>

enum class STATUS : char;

class TaskBuilder final : public RunnableT<TaskBuilder> {

  STATUS compressEncryptSubtask(std::string& data, bool alldone);
  int copyRequestWithId(char* dst, std::string_view line);

  std::ifstream _input;
  std::deque<Subtask> _subtasks;
  int _requestIndex = 0;
  int _nextIdSz = 4;
  std::mutex _mutex;
  std::condition_variable _condition;
  void run() override;
  bool start() override { return true; }
 public:
  TaskBuilder();
  ~TaskBuilder() override;
  void stop() override {}
  STATUS getSubtask(Subtask& task);
  STATUS createSubtask();
};
