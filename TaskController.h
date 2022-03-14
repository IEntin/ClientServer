/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include "Header.h"
#include "MemoryPool.h"
#include <atomic>
#include <barrier>
#include <memory>
#include <vector>

using Batch = std::vector<std::string>;

using TaskPtr = std::shared_ptr<class Task>;

using ProcessRequest = std::string (*)(std::string_view);

using CompletionFunction = void (*) () noexcept;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

using TaskProcessorPtr = std::shared_ptr<class TaskProcessor>;

class TaskController : public std::enable_shared_from_this<TaskController> {
  TaskController(unsigned numberThreads, ProcessRequest processRequest, size_t bufferSize);
  void initialize();
  void push(TaskPtr task);
  void setNextTask();
  std::tuple<std::string_view, bool, size_t> next();
  void updateResponse(size_t index, std::string& rsp);
  bool stopped() const { return _stopped; }
  static TaskControllerPtr create(unsigned numberThreads,
				  ProcessRequest processRequest,
				  size_t bufferSize);
  static void onTaskFinish() noexcept;
  const unsigned _numberThreads;
  ProcessRequest _processRequest;
  std::barrier<CompletionFunction> _barrier;
  ThreadPool _threadPool;
  TaskPtr _task;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::queue<TaskPtr> _queue;
  std::atomic<bool> _stopped = false;
  MemoryPool _memoryPool;
 public:
  ~TaskController();
  void run() noexcept;
  void submitTask(const HEADER& header, std::vector<char>& input, Batch& response);
  void pushToThreadPool(TaskProcessorPtr processor);
  void setProcessMethod(ProcessRequest processMethod) { _processRequest = processMethod; }
  MemoryPool& getMemoryPool() { return _memoryPool; }
  void setMemoryPoolSize(size_t size);
  static TaskControllerPtr instance(unsigned numberThreads = 0,
				    ProcessRequest processRequest = nullptr,
				    size_t bufferSize = 0);
};

class TaskProcessor : public std::enable_shared_from_this<TaskProcessor>, public Runnable {
  TaskControllerPtr _controller;
 public:
  explicit TaskProcessor(TaskControllerPtr controller);
  ~TaskProcessor() override;
  void run() noexcept override;
};
