/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPoolSessions.h"
#include "Header.h"
#include "Logger.h"
#include "Server.h"
#include <cassert>

std::shared_ptr<KillThread> ThreadPoolSessions::_killThread = std::make_shared<KillThread>();

ThreadPoolSessions::ThreadPoolSessions(int maxNumberRunningTotal) :
  _maxNumberRunningTotal(maxNumberRunningTotal) {}

ThreadPoolSessions::~ThreadPoolSessions() {
  Trace << std::endl;
}

void ThreadPoolSessions::stop() {
  // wake up and join threads
  assert(!_stopFlag.test_and_set() && "repeated call");
  try {
    for (int i = 0; i < size(); ++i)
      push(_killThread);
    for (auto& thread : _threads)
      if (thread.joinable())
	thread.join();
  }
  catch (const std::system_error& e) {
    LogError << e.what() << std::endl;
    return;
  }
  Trace << "... _threads joined ..." << std::endl;
}

void ThreadPoolSessions::createThread() {
  _threads.emplace_back([this] () {
    while (true) {
      // additional scope for fast recycling
      // of the finished runnable
      {
	// this blocks waiting for a new runnable
	RunnablePtr runnable = get();
	if (!runnable)
	  continue;
	// this kills the thread
	if (runnable->killThread())
	  break;
	runnable->run();
      }
    }
  });
}

void ThreadPoolSessions::push(RunnablePtr runnable, std::function<bool(RunnablePtr)> func) {
  if (!runnable)
    return;
  std::lock_guard lock(_queueMutex);
  // need one more thread
  bool condition1 = Server::totalSessions() > size();
  // can run one more of type
  bool condition2 = runnable->getNumberObjects() <= runnable->_maxNumberRunningByType;
  // can run one more of any
  bool condition3 = runnable->_numberRunningTotal < _maxNumberRunningTotal;
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

RunnablePtr ThreadPoolSessions::get() {
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

void ThreadPoolSessions::removeFromQueue(RunnablePtr toRemove) {
  std::unique_lock lock(_queueMutex);
  for (auto it = _queue.begin(); it < _queue.end(); ++it) {
    if (*it == toRemove) {
      _queue.erase(it);
      break;
    }
  }
}
