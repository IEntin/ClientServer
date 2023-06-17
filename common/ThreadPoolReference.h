/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"

template <typename T> class ThreadPoolReference {
 public:
  ThreadPoolReference(T& threadPool) : _threadPool(threadPool) {
    _threadPool.increment();
  }
  ~ThreadPoolReference() {
    _threadPool.decrement();
  }
  std::atomic<int>& totalNumberObjects() {
    return _threadPool.totalNumberObjects();
  }
  void push(RunnablePtr runnable) {
    _threadPool.push(runnable);
  }
 private:
  T& _threadPool;
};
