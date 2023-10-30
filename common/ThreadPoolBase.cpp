/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPoolBase.h"
#include "Logger.h"

ThreadPoolBase::ThreadPoolBase(int maxSize) : _maxSize(maxSize) {}

ThreadPoolBase::~ThreadPoolBase() {
  stop();
  Trace << '\n';
}

void ThreadPoolBase::stop() {
  // It is safe and cheap to call this more than once.
  // Wake up and join threads
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
	else
	  thread.join();
      }
      it = _threads.erase(it);
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
      struct Decrement {
	Decrement(ThreadPoolBase* threadPool) : _threadPool(threadPool) {}
	~Decrement() {
	  _threadPool->_totalNumberObjects--;
	}
	ThreadPoolBase* _threadPool = nullptr;
      } decrement(this);
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

void ThreadPoolBase::push(RunnablePtr runnable) {
  std::lock_guard lock(_queueMutex);
  increment();
  if (_totalNumberObjects > size()) {
    createThread();
    Debug << "numberOfThreads " << size() << ' ' << runnable->getType() << '\n';
  }
  _queue.emplace_back(runnable);
  _queueCondition.notify_one();
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
