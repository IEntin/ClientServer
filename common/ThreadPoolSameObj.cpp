/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPoolSameObj.h"
#include "Logger.h"

/*
This thread pool works with single runnable type
with a constraint on the number of running objects
of this type. It creates new threads on demand not
creating redundant threads.
*/
ThreadPoolSameObj::ThreadPoolSameObj(int maxSize) : ThreadPoolBase(maxSize) {}

ThreadPoolSameObj::~ThreadPoolSameObj() {
  Trace << '\n';
}

void ThreadPoolSameObj::push(RunnablePtr runnable) {
  std::lock_guard lock(_queueMutex);
  increment();
  bool condition1 = _totalNumberObjects > size();
  bool condition2 = size() < _maxSize;
  if (condition1 && condition2) {
    createThread();
    Debug << "numberOfThreads " << size() << ' ' << runnable->getType() << '\n';
  }
  else if (!condition2)
    runnable->_status = STATUS::MAX_OBJECTS_OF_TYPE;
  runnable->checkCapacity();
  _queue.emplace_back(runnable);
  _queueCondition.notify_one();
}
