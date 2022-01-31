/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Task.h"
#include <barrier>

using ProcessRequest = std::string (*)(std::string_view);

using CompletionFunction = void (*) () noexcept;

class TaskThreadPool {
  TaskThreadPool() = delete;
  ~TaskThreadPool() = delete;
  static std::vector<std::thread> _taskThreads;
 public:
  static void start(unsigned numberThreads, ProcessRequest processRequest);
  static void stop();
};

class TaskThread {
public:
  TaskThread(ProcessRequest processRequest);
  ~TaskThread();
  void operator()() noexcept;
  static void createBarrier(unsigned numberThreads);
private:
  ProcessRequest _processRequest;
  static void onTaskFinish() noexcept;
  static TaskPtr _task;
  static std::unique_ptr<std::barrier<CompletionFunction>> _barrier;
};
