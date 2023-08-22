/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolSameObj.h"
#include <barrier>
#include <map>
#include <queue>
#include <vector>

using Response = std::vector<std::string>;
using TaskPtr = std::shared_ptr<class Task>;
using TaskControllerPtr = std::shared_ptr<class TaskController>;
using TaskControllerWeakPtr = std::weak_ptr<class TaskController>;
struct ServerOptions;

class TaskController {
  enum Phase { PREPROCESSTASK, PROCESSTASK };
  class Worker : public RunnableT<Worker> {
    Worker(const Worker& other) = delete;
    Worker& operator =(const Worker& other) = delete;
    bool start() override { return true; }
    void stop() override {}
    void run() noexcept override;
    TaskControllerWeakPtr _taskController;
  public:
    explicit Worker(TaskControllerWeakPtr taskController);
    ~Worker() override {}
  };
  using CompletionFunction = void (*) () noexcept;
  TaskController(const TaskController& other) = delete;
  TaskController& operator =(const TaskController& other) = delete;
  bool start();
  void stop();
  void push(TaskPtr task);
  void setNextTask();
  static void onTaskCompletion() noexcept;
  void onCompletion();
  const ServerOptions& _options;
  const bool _sortInput;
  std::atomic<bool> _stopped = false;
  std::barrier<CompletionFunction> _barrier;
  ThreadPoolSameObj _threadPool;
  TaskPtr _task;
  std::condition_variable _queueCondition;
  std::queue<TaskPtr> _queue;
  Phase _phase = PREPROCESSTASK;
  std::mutex _queueMutex;
  static TaskControllerPtr _single;
 public:
  TaskController(const ServerOptions& options);
  ~TaskController();
  void processTask(const HEADER& header, std::string_view input, Response& response);
  static bool create(const ServerOptions& options);
  static void destroy();
  static TaskControllerWeakPtr weakInstance();
  static bool isDiagnosticsEnabled();
};
