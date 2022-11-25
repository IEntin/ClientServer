/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "ObjectCounter.h"
#include "Runnable.h"
#include "Strategy.h"
#include "ThreadPool.h"
#include <barrier>
#include <queue>
#include <vector>

using Response = std::vector<std::string>;

using TaskPtr = std::shared_ptr<class Task>;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

using TaskControllerWeakPtr = std::weak_ptr<class TaskController>;

struct ServerOptions;

class TaskController : public std::enable_shared_from_this<TaskController> {
  enum Phase { PREPROCESSTASK, PROCESSTASK };
  class Worker : public std::enable_shared_from_this<Worker>, public Runnable {
    Worker(const Worker& other) = delete;
    Worker& operator =(const Worker& other) = delete;
    bool start() override;
    void run() noexcept override;
    unsigned getNumberObjects() const override;
    TaskControllerWeakPtr _taskController;
    ObjectCounter<Worker> _objectCounter;
  public:
    explicit Worker(TaskControllerWeakPtr taskController);
    ~Worker() override;
    void stop() override;
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
  const ServerOptions& _options;
  const bool _sortInput;
  static void onTaskCompletion() noexcept;
  void onCompletion();
  std::atomic<bool> _stopped = false;
  std::barrier<CompletionFunction> _barrier;
  ThreadPool _threadPool;
  TaskPtr _task;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::queue<TaskPtr> _queue;
  Phase _phase = PREPROCESSTASK;
  Strategy& _strategy;
  std::atomic<unsigned> _totalSessions = 0;
  static TaskControllerPtr _single;
 public:
  TaskController(const ServerOptions& options);
  ~TaskController();
  void processTask(const HEADER& header, std::vector<char>& input, Response& response);
  static bool create(ServerOptions& options);
  static void destroy();
  static TaskControllerWeakPtr weakInstance();
  static bool isDiagnosticsEnabled();
  static std::atomic<unsigned>& totalSessions();
};
