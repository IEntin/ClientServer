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

using TaskControllerPtr = std::shared_ptr<class TaskController>;

using TaskThreadPtr = std::shared_ptr<class TaskThread>;

class TaskController : public std::enable_shared_from_this<TaskController> {
  friend class TaskThread;
  const unsigned _numberThreads;
  ProcessRequest _processRequest;
  std::barrier<CompletionFunction> _barrier;
  ThreadPool _threadPool;
  std::atomic<bool> _stopped = false;
  bool stopped() const { return _stopped; }
  static void onTaskFinish() noexcept;
 public:
  TaskController(unsigned numberThreads, ProcessRequest processRequest);
  ~TaskController() = default;
  void start();
  void stop();
};

class TaskThread : public std::enable_shared_from_this<TaskThread>, public Runnable {
  friend class TaskController;
  TaskControllerPtr _pool;
  ProcessRequest _processRequest;
 public:
  TaskThread(TaskControllerPtr pool, ProcessRequest processRequest);
  ~TaskThread() override;
  void run() noexcept override;
  void startInstance();
};
