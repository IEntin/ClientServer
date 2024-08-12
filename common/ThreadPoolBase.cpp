/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPoolBase.h"

#include <cassert>

#include "Logger.h"

ThreadPoolBase::ThreadPoolBase(int maxSize) : _maxSize(maxSize) {}

ThreadPoolBase::~ThreadPoolBase() {
  assert(_threads.empty());
  Trace << '\n';
}

void ThreadPoolBase::stop() {
  try {
    {
      std::lock_guard lock(_queueMutex);
      _stopped = true;
      _queueCondition.notify_all();
    }
    for (auto it = _threads.begin(); it != _threads.end(); ) {
      auto& thread = *it;
      if (thread.joinable()) {
	// prevents core dump in case of a code defect.
	// Should not happen.
	if (thread.get_id() == std::this_thread::get_id()) {
	  thread.detach();
	  Warn << "thread detached." << '\n';
	}
	else {
	  thread.join();
	  it = _threads.erase(it);
	}
      }
    }
    std::deque<RunnablePtr>().swap(_queue);
  }
  catch (const std::system_error& e) {
    LogError << e.what() << '\n';
    return;
  }
  Trace << "... _threads joined ..." << '\n';
}

void  ThreadPoolBase::createThread() {
  _threads.emplace_back([this] () {
    while (true) {
      struct Finally {
	Finally(ThreadPoolBase* threadPool) : _threadPool(threadPool) {}
	~Finally() {
	  if (_threadPool->_totalNumberObjects > 0)
	    --_threadPool->_totalNumberObjects;
	}
	ThreadPoolBase* _threadPool = nullptr;
      } finally(this);
      // this blocks waiting for a new runnable
      RunnablePtr runnable = get();
      if (_stopped)
	return;
      if (!runnable)
	continue;
      try {
	runnable->run();
      }
      catch (const std::exception& e) {
	runnable->stop();
	LogError << e.what() << '\n';
      }
    }
  });
}

STATUS ThreadPoolBase::push(RunnablePtr runnable) {
  std::lock_guard lock(_queueMutex);
  ++_totalNumberObjects;
  if (_totalNumberObjects > size()) {
    createThread();
    Debug << "numberOfThreads=" << size() << ' ' << runnable->getType() << '\n';
  }
  _queue.emplace_back(runnable);
  _queueCondition.notify_one();
  return STATUS::NONE;
}

RunnablePtr ThreadPoolBase::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty() || _stopped; });
  if (_stopped)
    return RunnablePtr();
  RunnablePtr runnable = _queue.front();
  _queue.pop_front();
  return runnable;
}
