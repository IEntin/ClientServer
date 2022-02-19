/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include <atomic>
#include <barrier>
#include <memory>
#include <vector>

using ProcessRequest = std::string (*)(std::string_view);

using CompletionFunction = void (*) () noexcept;

using TaskThreadPoolPtr = std::shared_ptr<class TaskThreadPool>;

using TaskThreadPtr = std::shared_ptr<class TaskThread>;

class TaskThreadPool : public std::enable_shared_from_this<TaskThreadPool> {
  friend class TaskThread;
  const unsigned _numberThreads;
  ProcessRequest _processRequest;
  std::barrier<CompletionFunction> _barrier;
  ThreadPool _threadPool;
  std::atomic<bool> _stopped = false;
  bool stopped() const { return _stopped; }
  static void onTaskFinish() noexcept;
 public:
  TaskThreadPool(unsigned numberThreads, ProcessRequest processRequest);
  ~TaskThreadPool() = default;
  void start();
  void stop();
};

class TaskThread : public std::enable_shared_from_this<TaskThread>, public Runnable {
  friend class TaskThreadPool;
  TaskThreadPoolPtr _pool;
  ProcessRequest _processRequest;
 public:
  TaskThread(TaskThreadPoolPtr pool, ProcessRequest processRequest);
  ~TaskThread() override;
  void run() noexcept override;
  void startInstance();
};
