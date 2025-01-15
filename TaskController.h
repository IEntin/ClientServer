/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <barrier>
#include <queue>

#include "ThreadPoolBase.h"

using TaskPtr = std::shared_ptr<class Task>;
using TaskControllerPtr = std::shared_ptr<class TaskController>;
using TaskControllerWeakPtr = std::weak_ptr<class TaskController>;

class TaskController {
  enum Phase { PREPROCESSTASK, PROCESSTASK };
  class Worker : public Runnable {
    bool start() override { return true; }
    void stop() override {}
    void run() noexcept override;
    TaskControllerWeakPtr _taskController;
  public:
    explicit Worker(TaskControllerWeakPtr taskController);
  };
  using CompletionFunction = void (*) () noexcept;
  bool start();
  void stop();
  void push(TaskPtr task);
  void setNextTask();
  static void onTaskCompletion() noexcept;
  void onCompletion();
  std::atomic<bool> _stopped = false;
  std::barrier<CompletionFunction> _barrier;
  ThreadPoolBase _threadPool;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::queue<TaskPtr> _queue;
  TaskPtr _task;
  static TaskControllerPtr _single;
  static Phase _phase;
 public:
  TaskController();
  ~TaskController() = default;
  void processTask(TaskPtr task);
  static bool create();
  static void destroy();
  static TaskControllerWeakPtr getWeakPtr();
};
