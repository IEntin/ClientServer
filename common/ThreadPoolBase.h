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
  ThreadPoolBase& operator =(const ThreadPoolBase& other) = delete;
  std::vector<std::jthread> _threads;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::deque<RunnablePtr> _queue;
  std::atomic_flag _stopFlag;
  static std::shared_ptr<class KillThread> _killThread;
public:
  ThreadPoolBase();
  virtual ~ThreadPoolBase();
  ThreadPoolBase(const ThreadPoolBase& other) = delete;
  void stop();
  void push(RunnablePtr runnable);
  virtual RunnablePtr get();
  int size() const { return _threads.size(); }
  void removeFromQueue(RunnablePtr toRemove);
  // used in tests
  std::vector<std::jthread>& getThreads() { return _threads; }
  static inline std::atomic<int> _numberRelatedObjects = 0;
};
