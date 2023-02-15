/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPoolBase.h"
#include "Logger.h"
#include <cassert>

std::shared_ptr<KillThread> ThreadPoolBase::_killThread = std::make_shared<KillThread>();

ThreadPoolBase::ThreadPoolBase(int maxSize) : _maxSize(maxSize) {}

ThreadPoolBase::~ThreadPoolBase() {
  Trace << std::endl;
}

void ThreadPoolBase::stop() {
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

void  ThreadPoolBase::createThread() {
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

void ThreadPoolBase::push(RunnablePtr runnable) {
  if (!runnable)
    return;
  std::lock_guard lock(_queueMutex);
  if (_numberRelatedObjects > size()) {
    createThread();
    Debug << "numberOfThreads " << size() << ' ' << runnable->getType() << std::endl;
  }
  _queue.push_back(runnable);
  _queueCondition.notify_all();
}

RunnablePtr ThreadPoolBase::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  RunnablePtr runnable = _queue.front();
  _queue.pop_front();
  return runnable;
}

void ThreadPoolBase::removeFromQueue(RunnablePtr toRemove) {
  std::unique_lock lock(_queueMutex);
  for (auto it = _queue.begin(); it < _queue.end(); ++it) {
    if (*it == toRemove) {
      _queue.erase(it);
      break;
    }
  }
}
