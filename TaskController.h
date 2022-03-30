/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "MemoryPool.h"
#include "ThreadPool.h"
#include <atomic>
#include <barrier>
#include <memory>
#include <vector>

using Batch = std::vector<std::string>;

using TaskPtr = std::shared_ptr<class Task>;

using ProcessRequest = std::string (*)(std::string_view);

using CompletionFunction = void (*) () noexcept;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

class TaskController : public std::enable_shared_from_this<TaskController>, public Runnable {
  TaskController(unsigned numberThreads, size_t bufferSize);
  void initialize();
  void push(TaskPtr task);
  void setNextTask();
  bool stopped() const { return _stopped; }
  static TaskControllerPtr create(unsigned numberThreads, size_t bufferSize);
  static void onTaskFinish() noexcept;
  const unsigned _numberThreads;
  std::barrier<CompletionFunction> _barrier;
  ThreadPool _threadPool;
  TaskPtr _task;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::queue<TaskPtr> _queue;
  std::atomic<bool> _stopped = false;
  MemoryPool _memoryPool;
  static bool _diagnosticsEnabled;
 public:
  ~TaskController() override;
  void run() noexcept override;
  void submitTask(const HEADER& header, std::vector<char>& input, Batch& response);
  MemoryPool& getMemoryPool() { return _memoryPool; }
  void setMemoryPoolSize(size_t size);
  static TaskControllerPtr instance(unsigned numberThreads = 0, size_t bufferSize = 0);
  static bool isDiagnosticsEnabled() { return _diagnosticsEnabled; }
};
