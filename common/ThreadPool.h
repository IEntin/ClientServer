/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <condition_variable>
#include <memory>
#include <queue>
#include <thread>
#include <vector>
#include "Runnable.h"

enum class PROBLEMS : char;

using RunnablePtr = std::shared_ptr<class Runnable>;

using ThreadPoolPtr = std::shared_ptr<class ThreadPool>;

class ThreadPool : public std::enable_shared_from_this<ThreadPool> {
  void createThread();
  ThreadPool& operator =(const ThreadPool& other) = delete;
  std::vector<std::jthread> _threads;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::queue<RunnablePtr> _queue;
  const unsigned _maxNumberThreads;
  static std::shared_ptr<class KillThread> _killThread;
 public:
  explicit ThreadPool(unsigned maxNumberThreads = 0);
  ~ThreadPool();
  ThreadPool(const ThreadPool& other) = delete;
  void stop();
  PROBLEMS push(RunnablePtr runnable);
  RunnablePtr get();
  int size() const { return _threads.size(); }
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
