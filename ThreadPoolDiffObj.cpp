/*
 *  Copyright (C) 2021 Ilya Entin
 */

/*
This thread pool works with any number of different
runnable types with constraints on the number of
running objects of every type and on the total number
of running objects. It creates threads on demand
not creating redundant threads.
*/

#include "ThreadPoolDiffObj.h"
#include "Header.h"
#include "Logger.h"

ThreadPoolDiffObj::ThreadPoolDiffObj(int maxSize, std::function<bool(RunnablePtr)> func) :
  ThreadPoolBase(maxSize),
  _func(func) {}

ThreadPoolDiffObj::~ThreadPoolDiffObj() {
  Trace << std::endl;
}

void ThreadPoolDiffObj::push(RunnablePtr runnable) {
  if (!runnable)
    return;
  std::lock_guard lock(_queueMutex);
  // need one more thread
  bool condition1 = _numberRelatedObjects > size();
  // can run one more of type
  bool condition2 = runnable->getNumberObjects() <= runnable->_maxNumberRunningByType;
  if (condition1 && condition2 && size() < _maxSize) {
    createThread();
    Debug << "numberOfThreads " << size() << ' ' << runnable->getType() << std::endl;
  }
  else {
    if (!condition2)
      runnable->_status = STATUS::MAX_SPECIFIC_OBJECTS;
    else if (_numberRelatedObjects > _maxSize)
      runnable->_status = STATUS::MAX_TOTAL_OBJECTS;
  }
  runnable->checkCapacity();
  if (_func)
    _func(runnable);
  _queue.push_back(runnable);
  _queueCondition.notify_one();
}

RunnablePtr ThreadPoolDiffObj::get() {
  std::unique_lock lock(_queueMutex);
  while (true) {
    _queueCondition.wait(lock, [this] { return !_queue.empty(); });
    for (auto it = _queue.begin(); it < _queue.end(); ++it) {
      RunnablePtr runnable = *it;
      if (runnable->getNumberRunningByType() < runnable->_maxNumberRunningByType) {
	_queue.erase(it);
	return runnable;
      }
    }
  }
}
