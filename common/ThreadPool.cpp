/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPool.h"
#include "Utility.h"

std::shared_ptr<KillThread> ThreadPool::_killThread = std::make_shared<KillThread>();

ThreadPool::ThreadPool(unsigned maxSize) : _maxSize(maxSize) {
  for (unsigned i = 0; i < _maxSize; ++i)
    createThread();
}

ThreadPool::~ThreadPool() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void ThreadPool::stop() {
  // wake up and join threads
  try {
    for (unsigned i = 0; i < size(); ++i)
      push(_killThread);
    for (auto& thread : _threads)
      if (thread.joinable())
	thread.join();
  }
  catch (const std::system_error& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
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

PROBLEMS ThreadPool::push(RunnablePtr runnable) {
  if (!runnable)
    return PROBLEMS::NONE;
  PROBLEMS problem = PROBLEMS::NONE;
  std::lock_guard lock(_queueMutex);
  if (_maxSize == 0 && runnable->getNumberObjects() > size()) {
    // this works if all runnables are of the same derived class
    createThread();
    auto& obj = *runnable.get();
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":class-"
	 << typeid(obj).name() << ",number objects=" << runnable->getNumberObjects()
	 << ",number threads=" << size() << std::endl;
  }
  problem = runnable->checkCapacity();
  _queue.push_back(runnable);
  _queueCondition.notify_all();
  return problem;
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
