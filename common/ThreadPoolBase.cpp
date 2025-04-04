/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPoolBase.h"

ThreadPoolBase::ThreadPoolBase(int maxSize) : _maxSize(maxSize) {}

ThreadPoolBase::~ThreadPoolBase() {
  if (!_threads.empty()) {
    LogError << "_threads are not empty; exiting.\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::exit(1);
  }
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
    while (!_stopped) {
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
      if (runnable) {
	try {
	  runnable->run();
	}
	catch (const std::exception& e) {
	  runnable->stop();
	  LogError << e.what() << '\n';
	}
      }
    }
  });
}

STATUS ThreadPoolBase::push(RunnablePtr runnable) {
  std::lock_guard lock(_queueMutex);
  if (size() >= _maxSize) {
    runnable->_status = STATUS::MAX_TOTAL_OBJECTS;
    return STATUS::MAX_TOTAL_OBJECTS;
  }
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
  for (auto it = _queue.begin(); it != _queue.end(); ++it) {
    RunnablePtr runnable = *it;
    if (!runnable->_stopped && _totalNumberObjects < _maxSize) {
      _queue.erase(it);
      return runnable;
    }
  }
  return RunnablePtr();
}
