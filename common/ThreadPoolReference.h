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

private:
  T& _threadPool;
};
