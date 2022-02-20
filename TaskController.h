/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPool.h"
#include "Header.h"
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
  friend class TaskProcessor;
  const unsigned _numberThreads;
  ProcessRequest _processRequest;
  std::barrier<CompletionFunction> _barrier;
  ThreadPool _threadPool;
  TaskPtr _task;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
   std::queue<TaskPtr> _queue;
  std::atomic<bool> _stopped = false;
  bool stopped() const { return _stopped; }
  static TaskControllerPtr _instance;
  void startInstance();
  void push(TaskPtr task);
  void setNew();
  std::tuple<std::string_view, bool, size_t> next();
  void updateResponse(size_t index, std::string& rsp);
  void stopInstance();
  static void onTaskFinish() noexcept;
 public:
  TaskController(unsigned numberThreads, ProcessRequest processRequest);
  ~TaskController() = default;
  static void processTask(const HEADER& header, std::vector<char>& input, Batch& response);
  static void start(unsigned numberThreads, ProcessRequest processRequest);
  static void stop();
};

class TaskProcessor : public std::enable_shared_from_this<TaskProcessor>, public Runnable {
  friend class TaskController;
  TaskControllerPtr _controller;
  ProcessRequest _processRequest;
 public:
  TaskProcessor(TaskControllerPtr controller, ProcessRequest processRequest);
  ~TaskProcessor() override;
  void run() noexcept override;
  void startInstance();
};
