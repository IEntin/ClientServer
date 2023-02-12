/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolBase.h"

class ThreadPoolReference {
 public:
  ThreadPoolReference(ThreadPoolBase& threadPool) : _threadPool(threadPool) {
    _threadPool.increment();
  }
  ~ThreadPoolReference() {
    _threadPool.decrement();
  }
  std::atomic<int>& numberRelatedObjects() {
    return _threadPool.numberRelatedObjects();
  }
  void push(RunnablePtr runnable) {
    _threadPool.push(runnable);
  }
 private:
  ThreadPoolBase& _threadPool;
};
