/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPool.h"
#include "Header.h"
#include "Logger.h"

ThreadPool::ThreadPool(int maxSize) :
  _maxSize(maxSize) {}

ThreadPool::~ThreadPool() {
  Trace << std::endl;
}

void ThreadPool::push(RunnablePtr runnable) {
  if (!runnable)
    return;
  std::lock_guard lock(_queueMutex);
  bool condition1 = runnable->getNumberObjects() > size();
  bool condition2 = size() < _maxSize;
  if (condition1 && condition2) {
    createThread();
    Debug << "numberOfThreads " << size() << ' ' << runnable->getType() << std::endl;
  }
  else if (!condition2)
    runnable->_status = STATUS::MAX_SPECIFIC_OBJECTS;
  runnable->checkCapacity();
  _queue.push_back(runnable);
  _queueCondition.notify_all();
}
