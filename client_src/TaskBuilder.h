/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "Subtask.h"
#include <condition_variable>
#include <deque>
#include <fstream>
#include <mutex>

enum class STATUS : char;

class TaskBuilder final : public Runnable {

  STATUS compressEncryptSubtask(bool alldone);
  void copyRequestWithId(std::string_view line);

  std::ifstream _input;
  std::deque<Subtask> _subtasks;
  static Subtask _emptySubtask;
  std::atomic<unsigned> _requestIndex = 0;
  std::string _aggregate;
  std::string _line;
  std::atomic<unsigned> _subtaskIndexOut = 0;
  std::mutex _mutex;
  std::condition_variable _condition;
  bool _resume = false;
  void run() override;
  bool start() override { return true; }
 public:
  TaskBuilder();
  ~TaskBuilder() override;
  Subtask& getSubtask();
  STATUS createSubtask();
  void stop() override;
  void resume();
};
