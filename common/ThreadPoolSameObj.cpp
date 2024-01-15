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
  bool condition = _totalNumberObjects > size();
  if (condition && runnable->checkCapacity()) {
    createThread();
    Debug << "numberOfThreads=" << size() << ' ' << runnable->getType() << '\n';
  }
  _queue.emplace_back(runnable);
  _queueCondition.notify_one();
}

RunnablePtr ThreadPoolSameObj::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty() || _stopped; });
  if (_stopped)
    return RunnablePtr();
  for (auto it = _queue.begin(); it != _queue.end(); ++it) {
    RunnablePtr runnable = *it;
    if (runnable->checkCapacity()) {
      _queue.erase(it);
      return runnable;
    }
  }
  return RunnablePtr();
}
