/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <condition_variable>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

using RunnablePtr = std::shared_ptr<class Runnable>;
using ThreadPoolPtr = std::shared_ptr<class ThreadPool>;

class Runnable {
 public:
  Runnable() = default;
  virtual ~Runnable() {}
  virtual void run() = 0;
};

class ThreadPool : public std::enable_shared_from_this<ThreadPool> {
  std::vector<std::thread> _threads;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::queue<RunnablePtr> _queue;
 public:
  ThreadPool(unsigned numberThreads);
  ~ThreadPool();
  void stop();
  void push(RunnablePtr runnable);
  RunnablePtr get();
  // used in tests
  std::vector<std::thread>& getThreads() { return _threads; }
};