/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Strategy.h"
#include "Runnable.h"
#include "ThreadPool.h"
#include <atomic>
#include <barrier>
#include <memory>
#include <queue>
#include <vector>

enum class COMPRESSORS : int;

using Response = std::vector<std::string>;

using TaskPtr = std::shared_ptr<class Task>;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

using HEADER = std::tuple<ssize_t, ssize_t, COMPRESSORS, bool, unsigned short, bool>;

struct ServerOptions;

class TaskController : public std::enable_shared_from_this<TaskController>, public Runnable {
  enum Phase { PREPROCESSTASK, PROCESSTASK };
  enum Operations { KEEP, DESTROY, RESET };
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
  std::barrier<CompletionFunction> _barrier;
  ThreadPool _threadPool;
  TaskPtr _task;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::queue<TaskPtr> _queue;
  static Phase _phase;
  static bool _diagnosticsEnabled;
  Strategy& _strategy;
 public:
  ~TaskController() override;
  bool start() override;
  void stop() override;
  void run() noexcept override;
  void submitTask(const HEADER& header, std::vector<char>& input, Response& response);
  static TaskControllerPtr instance(const ServerOptions* options = nullptr, Operations op = KEEP);
  static bool isDiagnosticsEnabled() { return _diagnosticsEnabled; }
};
