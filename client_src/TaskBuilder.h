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

class TaskBuilder final : public Runnable {

  STATUS compressEncryptSubtask(bool alldone);
  void copyRequestWithId(std::string_view line);

  std::ifstream _input;
  std::deque<Subtask> _subtasks;
  std::atomic<int> _requestIndex = 0;
  std::string _aggregate;
  std::string _line;
  std::mutex _mutex;
  std::condition_variable _conditionDone;
  std::condition_variable _conditionResume;
  bool _resume = false;
  void run() override;
  bool start() override { return true; }
 public:
  TaskBuilder();
  ~TaskBuilder() override;
  STATUS getSubtask(Subtask& task);
  STATUS createSubtask();
  void stop() override;
  void resume();
};
