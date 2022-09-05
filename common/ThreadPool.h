/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <condition_variable>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

enum class PROBLEMS : char;

using RunnablePtr = std::shared_ptr<class Runnable>;

using ThreadPoolPtr = std::shared_ptr<class ThreadPool>;

class ThreadPool : public std::enable_shared_from_this<ThreadPool> {
  ThreadPool& operator =(const ThreadPool& other) = delete;
  std::vector<std::jthread> _threads;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::queue<RunnablePtr> _queue;
  const unsigned _maxNumberThreads;
 public:
  explicit ThreadPool(unsigned maxNumberThreads);
  ~ThreadPool();
  ThreadPool(const ThreadPool& other) = delete;
  void stop();
  PROBLEMS push(RunnablePtr runnable);
  RunnablePtr get();
  int size() const { return _threads.size(); }
  // used in tests
  std::vector<std::jthread>& getThreads() { return _threads; }
};
