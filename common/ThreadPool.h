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
  const unsigned _maxSize;
  static std::shared_ptr<class KillThread> _killThread;
 public:
  // default 0 means can grow
  explicit ThreadPool(unsigned maxSize = 0);
  ~ThreadPool();
  ThreadPool(const ThreadPool& other) = delete;
  void stop();
  void push(RunnablePtr runnable);
  RunnablePtr get();
  unsigned size() const { return _threads.size(); }
  unsigned maxSize() const { return _maxSize; }
  void removeFromQueue(RunnablePtr toRemove);
  // used in tests
  std::vector<std::jthread>& getThreads() { return _threads; }
};

class KillThread : public Runnable {
  void run() override {}
  bool start() override { return true; }
  void stop() override {}
 public:
  ~KillThread() override {}
  bool killThread() const override { return true; }
};
