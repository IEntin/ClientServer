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

class ThreadPool : public std::enable_shared_from_this<ThreadPool> {
  ThreadPool& operator =(const ThreadPool& other) = delete;
  void start(int numberThreads);
  std::vector<std::thread> _threads;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::queue<RunnablePtr> _queue;
 public:
  explicit ThreadPool(int numberThreads);
  ~ThreadPool();
  ThreadPool(const ThreadPool& other) = delete;
  void stop();
  void push(RunnablePtr runnable);
  RunnablePtr get();
  int size() const { return _threads.size(); }
  // used in tests
  std::vector<std::thread>& getThreads() { return _threads; }
};
