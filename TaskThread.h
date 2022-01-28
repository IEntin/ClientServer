/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Task.h"
#include <barrier>
#include <vector>

using ProcessRequest = std::string (*)(std::string_view, bool diagnostics);

using CompletionFunction = void (*) () noexcept;

class TaskThread {
public:
  static bool startThreads(ProcessRequest processRequest);
  static void joinThreads();
private:
  TaskThread() = delete;
  ~TaskThread() = delete;
  static void onTaskFinish() noexcept;
  static void processTask(TaskPtr& task, ProcessRequest processRequest);
  static TaskPtr _task;
  static unsigned _numberThreads;
  static std::vector<std::thread> _taskThreads;
  static std::barrier<CompletionFunction> _barrier;
  static bool _diagnostics;
};
