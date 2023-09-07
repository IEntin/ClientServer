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
  void increment() { _totalNumberObjects++; }
  std::vector<std::thread> _threads;
  std::mutex _queueMutex;
  std::condition_variable _queueCondition;
  std::deque<RunnablePtr> _queue;
  std::atomic<bool> _stopped = false;
  std::atomic<unsigned> _totalNumberObjects = 0;
  const unsigned _maxSize;
public:
  ThreadPoolBase(int maxSize = MAX_NUMBER_THREADS_DEFAULT);
  virtual ~ThreadPoolBase();
  void stop();
  virtual void push(RunnablePtr runnable);
  virtual RunnablePtr get();
  unsigned size() const { return _threads.size(); }
  unsigned maxSize() const { return _maxSize; }
};
