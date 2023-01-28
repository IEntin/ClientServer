/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <condition_variable>
#include <deque>
#include <thread>
#include <vector>

class ThreadPool {
  void createThread();
  ThreadPool& operator =(const ThreadPool& other) = delete;
  std::vector<std::jthread> _threads;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::deque<RunnablePtr> _queue;
  const int _maxSize;
  static std::shared_ptr<class KillThread> _killThread;
  std::atomic_flag _stopFlag = ATOMIC_FLAG_INIT;
  public:
  explicit ThreadPool(int maxSize = MAX_NUMBER_THREADS_DEFAULT);
  ~ThreadPool();
  ThreadPool(const ThreadPool& other) = delete;
  void stop();
  void push(RunnablePtr runnable);
  RunnablePtr get();
  int size() const { return _threads.size(); }
  int maxSize() const { return _maxSize; }
  void removeFromQueue(RunnablePtr toRemove);
  // used in tests
  std::vector<std::jthread>& getThreads() { return _threads; }
};
