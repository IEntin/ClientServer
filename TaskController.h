/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolSameObj.h"
#include <boost/core/noncopyable.hpp>
#include <barrier>
#include <map>
#include <queue>
#include <vector>

using Response = std::vector<std::string>;
using TaskPtr = std::shared_ptr<class Task>;
using TaskControllerPtr = std::shared_ptr<class TaskController>;
using TaskControllerWeakPtr = std::weak_ptr<class TaskController>;

class TaskController : private boost:: noncopyable {
  enum Phase { PREPROCESSTASK, PROCESSTASK };
  class Worker : public Runnable {
    bool start() override { return true; }
    void stop() override {}
    void run() noexcept override;
    TaskControllerWeakPtr _taskController;
  public:
    explicit Worker(TaskControllerWeakPtr taskController);
    ~Worker() override {}
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
  ThreadPoolSameObj _threadPool;
  TaskPtr _task;
  std::condition_variable _queueCondition;
  std::queue<TaskPtr> _queue;
  std::mutex _queueMutex;
  static TaskControllerPtr _single;
  static Phase _phase;
 public:
  TaskController();
  ~TaskController();
  void processTask(const HEADER& header, std::string_view input, Response& response);
  static bool create();
  static void destroy();
  static TaskControllerWeakPtr weakInstance();
};
