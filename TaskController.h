/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Runnable.h"
#include "Strategy.h"
#include "ThreadPool.h"
#include <barrier>
#include <queue>
#include <vector>

using Response = std::vector<std::string>;

using TaskPtr = std::shared_ptr<class Task>;

using TaskProcessorPtr = std::shared_ptr<class TaskProcessor>;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

struct ServerOptions;

enum class TaskControllerOps : char { KEEP, DESTROY, RECREATE };

class TaskController : public std::enable_shared_from_this<TaskController> {
  enum Phase { PREPROCESSTASK, PROCESSTASK };
  using CompletionFunction = void (*) () noexcept;
  TaskController(const ServerOptions& options);
  TaskController(const TaskController& other) = delete;
  TaskController& operator =(const TaskController& other) = delete;
  void initialize();
  void push(TaskPtr task);
  void setNextTask();
  void wakeupThreads();
  static TaskControllerPtr create(const ServerOptions& options);
  const ServerOptions& _options;
  const bool _sortInput;
  static void onTaskCompletion() noexcept;
  void onCompletion();
  std::barrier<CompletionFunction> _barrier;
  ThreadPool _threadPool;
  TaskPtr _task;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::queue<TaskPtr> _queue;
  Phase _phase = PREPROCESSTASK;
  Strategy& _strategy;
  std::vector<TaskProcessorPtr> _processors;
  std::atomic<unsigned> _totalSessions = 0;
 public:
  ~TaskController();
  bool start();
  void stop();
  void run() noexcept;
  void processTask(const HEADER& header, std::vector<char>& input, Response& response);
  static TaskControllerPtr instance(const ServerOptions* options = nullptr,
				    TaskControllerOps op = TaskControllerOps::KEEP);
  static bool isDiagnosticsEnabled();
  static std::atomic<unsigned>& totalSessions();
};
