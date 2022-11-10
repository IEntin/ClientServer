/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPool.h"
#include "Utility.h"

std::shared_ptr<KillThread> ThreadPool::_killThread = std::make_shared<KillThread>();

ThreadPool::ThreadPool(unsigned maxSize) : _maxSize(maxSize) {}

ThreadPool::~ThreadPool() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void ThreadPool::stop() {
  // wake up and join threads
  assert(!_stopFlag.test_and_set() && "repeated call");
  try {
    for (unsigned i = 0; i < size(); ++i)
      push(_killThread);
    for (auto& thread : _threads)
      if (thread.joinable())
	thread.join();
  }
  catch (const std::system_error& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
  }
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " ... _threads joined ..." << std::endl;
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
  if (size() < _maxSize && runnable->getNumberObjects() > size()) {
    // this works if all runnables are of the same derived class
    createThread();
    const auto& refObject = *runnable;
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ":numberOfThreads " << size() << ' '
	 << typeid(refObject).name() << std::endl;
  }
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
