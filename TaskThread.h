/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Task.h"
#include <barrier>

using ProcessRequest = std::string (*)(std::string_view);

using CompletionFunction = void (*) () noexcept;

using TaskThreadPoolPtr = std::shared_ptr<class TaskThreadPool>;

using TaskThreadPtr = std::shared_ptr<class TaskThread>;

class TaskThreadPool : public std::enable_shared_from_this<TaskThreadPool> {
  friend class TaskThread;
  const unsigned _numberThreads;
  ProcessRequest _processRequest;
  std::barrier<CompletionFunction> _barrier;
  std::vector<TaskThreadPtr> _threads;
 public:
  TaskThreadPool(unsigned numberThreads, ProcessRequest processRequest);
  ~TaskThreadPool() = default;
  void start();
  void stop();
};

class TaskThread {
  friend class TaskThreadPool;
  class Runnable {
    TaskThreadPoolPtr _pool;
    std::reference_wrapper<std::barrier<CompletionFunction>> _barrier;
    ProcessRequest _processRequest;
  public:
    Runnable(TaskThreadPoolPtr pool, ProcessRequest processRequest);
    ~Runnable() = default;
    void operator()() noexcept;
  } _runnable;
  static TaskPtr _task;
  static void onTaskFinish() noexcept;
  std::thread _thread;
 public:
  TaskThread(TaskThreadPoolPtr pool, ProcessRequest processRequest);
  ~TaskThread();
};
