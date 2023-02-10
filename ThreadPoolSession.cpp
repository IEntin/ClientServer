/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPoolSession.h"
#include "Header.h"
#include "Logger.h"

ThreadPoolSession::ThreadPoolSession(int maxNumberRunningTotal) :
  _maxNumberRunningTotal(maxNumberRunningTotal) {}

ThreadPoolSession::~ThreadPoolSession() {
  Trace << std::endl;
}

void ThreadPoolSession::push(RunnablePtr runnable, std::function<bool(RunnablePtr)> func) {
  if (!runnable)
    return;
  std::lock_guard lock(_queueMutex);
  // need one more thread
  bool condition1 = _numberRelatedObjects > size();
  // can run one more of type
  bool condition2 = runnable->getNumberObjects() <= runnable->_maxNumberRunningByType;
  // can run one more of any
  bool condition3 = size() < _maxNumberRunningTotal;
  if (condition1 && condition2 && condition3) {
    createThread();
    Debug << "numberOfThreads " << size() << ' ' << runnable->getType() << std::endl;
  }
  else {
    if (!condition2)
      runnable->_status = STATUS::MAX_SPECIFIC_SESSIONS;
    else if (!condition3)
      runnable->_status = STATUS::MAX_TOTAL_SESSIONS;
  }
  runnable->checkCapacity();
  if (func)
    func(runnable);
  _queue.push_back(runnable);
  _queueCondition.notify_one();
}

RunnablePtr ThreadPoolSession::get() {
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
