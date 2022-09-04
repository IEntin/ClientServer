/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPool.h"
#include "Header.h"
#include "Runnable.h"
#include "Utility.h"

ThreadPool::ThreadPool(unsigned maxNumberThreads) : _maxNumberThreads(maxNumberThreads) {
  start();
}

ThreadPool::~ThreadPool() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void ThreadPool::start() {
  for (unsigned i = 0; i < _maxNumberThreads; ++i) {
    _threads.emplace_back([this] () {
			    while (true) {
			      // additional scope for fast recycling
			      // of the finished runnable
			      {
				// this blocks waiting for a new runnable
				RunnablePtr runnable = get();
				if (!runnable)
				  break;
				runnable->run();
			      }
			    }
			  });
  }
}

void ThreadPool::stop() {
  // wake up and join threads
  for (unsigned i = 0; i < _threads.size(); ++i)
    push(RunnablePtr());
  for (auto& thread : _threads)
    if (thread.joinable())
      thread.join();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " ... _threads joined ..." << std::endl;
}

PROBLEMS ThreadPool::push(RunnablePtr runnable) {
  std::lock_guard lock(_queueMutex);
  PROBLEMS problem = PROBLEMS::NONE;
  if (runnable)
    problem = runnable->checkCapacity();
  _queue.emplace_back(std::move(runnable));
  _queueCondition.notify_all();
  return problem;
}

RunnablePtr ThreadPool::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  RunnablePtr runnable = std::move(_queue.front());
  _queue.pop_front();
  return runnable;
}

void ThreadPool::remove(RunnablePtr runnable) {
  std::lock_guard lock(_queueMutex);
  for (auto it = _queue.begin(); it != _queue.end(); ) {
    RunnablePtr ptr = *it;
    if (ptr == runnable)
      it = _queue.erase(it);
    else
      ++it;
  }
}
