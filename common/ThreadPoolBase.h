/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <condition_variable>
#include <deque>
#include <thread>
#include <vector>

class ThreadPoolBase {
protected:
  void createThread();
  ThreadPoolBase(const ThreadPoolBase& other) = delete;
  ThreadPoolBase& operator =(const ThreadPoolBase& other) = delete;
  std::vector<std::jthread> _threads;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::deque<RunnablePtr> _queue;
  std::atomic_flag _stopFlag;
  std::atomic<int> _numberRelatedObjects = 0;
  const int _maxSize;
  static std::shared_ptr<class KillThread> _killThread;
public:
  ThreadPoolBase(int maxSize = MAX_NUMBER_THREADS_DEFAULT);
  virtual ~ThreadPoolBase();
  void stop();
  virtual void push(RunnablePtr runnable);
  virtual RunnablePtr get();
  int size() const { return _threads.size(); }
  void removeFromQueue(RunnablePtr toRemove);
  std::atomic<int>& numberRelatedObjects() { return _numberRelatedObjects; }
  void increment() { _numberRelatedObjects++; }
  void decrement() { _numberRelatedObjects--; }
  int maxSize() const { return _maxSize; }
  // used in tests
  std::vector<std::jthread>& getThreads() { return _threads; }
};
