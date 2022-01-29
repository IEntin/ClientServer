/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Task.h"
#include <barrier>
#include <vector>

using ProcessRequest = std::string (*)(std::string_view, bool diagnostics);

using CompletionFunction = void (*) () noexcept;

using TaskThreadPoolPtr = std::shared_ptr<class TaskThreadPool>;

using TaskThreadPtr = std::shared_ptr<class TaskThread>;

class TaskThreadPool : public std::enable_shared_from_this<TaskThreadPool> {
  unsigned _numberThreads;
  ProcessRequest _processRequest;
  std::barrier<CompletionFunction> _barrier;
  static TaskPtr _task;
  static bool _diagnostics;
  static std::vector<std::pair<TaskThreadPtr, std::thread>> _taskThreads;
 public:
  TaskThreadPool(unsigned numberThreads, ProcessRequest processRequest);
  ~TaskThreadPool() = default;
  void start();
  void stop();
  static void onTaskFinish() noexcept;
};

class TaskThread {
public:
  TaskThread(TaskThreadPoolPtr pool);
  ~TaskThread();

  void processTask(TaskPtr& task,
		   ProcessRequest processRequest,
		   std::barrier<CompletionFunction>& barrier,
		   bool& diagnostics);
private:
  TaskThreadPoolPtr _pool;
};
