/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "ThreadPool.h"
#include <barrier>
#include <map>
#include <queue>
#include <vector>

using Response = std::vector<std::string>;

using TaskPtr = std::shared_ptr<class Task>;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

using TaskControllerWeakPtr = std::weak_ptr<class TaskController>;

class Strategy;

struct ServerOptions;

class TaskController : public std::enable_shared_from_this<TaskController> {
  enum Phase { PREPROCESSTASK, PROCESSTASK };
  class Worker : public std::enable_shared_from_this<Worker>, public RunnableT<Worker> {
    Worker(const Worker& other) = delete;
    Worker& operator =(const Worker& other) = delete;
    bool start() override { return true; }
    void run() noexcept override;
    TaskControllerWeakPtr _taskController;
  public:
    explicit Worker(TaskControllerWeakPtr taskController);
    ~Worker() override {}
    void stop() override {}
  };
  using WorkerWeakPtr = std::weak_ptr<Worker>;
  using CompletionFunction = void (*) () noexcept;
  TaskController(const TaskController& other) = delete;
  TaskController& operator =(const TaskController& other) = delete;
  bool start();
  void stop();
  void push(TaskPtr task);
  void setNextTask();
  void wakeupThreads();
  static void onTaskCompletion() noexcept;
  void onCompletion();
  const ServerOptions& _options;
  const bool _sortInput;
  std::atomic<bool> _stopped = false;
  std::barrier<CompletionFunction> _barrier;
  ThreadPool _threadPool;
  TaskPtr _task;
  std::condition_variable _queueCondition;
  std::queue<TaskPtr> _queue;
  Phase _phase = PREPROCESSTASK;
  Strategy& _strategy;
  std::mutex _queueMutex;
  static TaskControllerPtr _single;
 public:
  TaskController(const ServerOptions& options, Strategy& strategy);
  ~TaskController();
  void processTask(const HEADER& header, std::vector<char>& input, Response& response);
  static bool create(ServerOptions& options, Strategy& strategy);
  static void destroy();
  static TaskControllerWeakPtr weakInstance();
  static bool isDiagnosticsEnabled();
};
