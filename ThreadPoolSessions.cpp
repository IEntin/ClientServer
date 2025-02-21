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

#include "ThreadPoolSessions.h"

#include "Logger.h"

ThreadPoolSessions::ThreadPoolSessions(int maxSize) :
  ThreadPoolBase(maxSize) {}

void ThreadPoolSessions::calculateStatus(RunnablePtr runnable) {
  ++_totalNumberObjects;
  // need one more thread ?
  bool condition1 = _totalNumberObjects > size();
  // can run one more of type ?
  bool condition2 = runnable->getNumberRunningByType() < runnable->_maxNumberRunningByType;
  if (condition1 && condition2 && size() < _maxSize) {
    createThread();
    Debug << "numberOfThreads=" << size() << ' ' << runnable->getType() << '\n';
  }
  else if (!condition2)
    runnable->_status = STATUS::MAX_OBJECTS_OF_TYPE;
  else if (runnable->_numberRunningTotal == _maxSize)
    runnable->_status = STATUS::MAX_TOTAL_OBJECTS;
  runnable->displayCapacityCheck(_totalNumberObjects);
}

STATUS ThreadPoolSessions::push(RunnablePtr runnable) {
  std::lock_guard lock(_queueMutex);
  _queue.emplace_back(runnable);
  _queueCondition.notify_one();
  return runnable->_status;
}

RunnablePtr ThreadPoolSessions::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty() || _stopped; });
  for (auto it = _queue.begin(); it != _queue.end(); ++it) {
    RunnablePtr runnable = *it;
    if (runnable->getNumberRunningByType() < runnable->_maxNumberRunningByType &&
	!runnable->_stopped) {
      _queue.erase(it);
      return runnable;
    }
  }
  return RunnablePtr();
}
