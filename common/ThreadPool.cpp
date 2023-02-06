/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPool.h"
#include "Header.h"
#include "Logger.h"
#include <cassert>

std::shared_ptr<KillThread> ThreadPool::_killThread = std::make_shared<KillThread>();

ThreadPool::ThreadPool(int maxSize) :
  _maxSize(maxSize) {}

ThreadPool::~ThreadPool() {
  Trace << std::endl;
}

void ThreadPool::stop() {
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

void  ThreadPool::createThread() {
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
    runnable->_status = STATUS::MAX_SPECIFIC_SESSIONS;
  runnable->checkCapacity();
  _queue.push_back(runnable);
  _queueCondition.notify_all();
}

RunnablePtr ThreadPool::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  RunnablePtr runnable = _queue.front();
  _queue.pop_front();
  return runnable;
}

void ThreadPool::removeFromQueue(RunnablePtr toRemove) {
  std::unique_lock lock(_queueMutex);
  for (auto it = _queue.begin(); it < _queue.end(); ++it) {
    if (*it == toRemove) {
      _queue.erase(it);
      break;
    }
  }
}
