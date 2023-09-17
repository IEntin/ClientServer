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

  STATUS compressEncryptSubtask(std::string_view data, bool alldone);
  void copyRequestWithId(std::string& aggregate, std::string_view line);

  std::ifstream _input;
  std::deque<Subtask> _subtasks;
  int _requestIndex = 0;
  std::mutex _mutex;
  std::condition_variable _condition;
  void run() override;
  bool start() override { return true; }
  void stop() override {}
  unsigned getNumberObjects() const override { return 1; }
  unsigned getNumberRunningByType() const override { return 1; }
  void displayCapacityCheck(std::atomic<unsigned>&) const override {}
 public:
  TaskBuilder();
  ~TaskBuilder() override;
  STATUS getSubtask(Subtask& task);
  STATUS createSubtask();
};
