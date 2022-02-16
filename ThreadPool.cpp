/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPool.h"
#include <iostream>

std::atomic<bool> ThreadPool::_destroyed = false;

ThreadPool::ThreadPool(unsigned numberThreads) {
  for (unsigned i = 0; i < numberThreads; ++i) {
    _threads.emplace_back([this] () {
			    while (true) {
			      // additional scope for the fast recycling
			      // of the finished runnable
			      {
				// this blocks waiting for the new runnable
				RunnablePtr runnable = get();
				if (!runnable)
				  break;
				runnable->run();
			      }
			    }
			  });
  }
}

ThreadPool::~ThreadPool() {
  _destroyed.store(true);
}

void ThreadPool::stop() {
  // wake up and join threads
  for (unsigned i = 0; i < _threads.size(); ++i)
    push(RunnablePtr());
  for (auto& thread : _threads)
    if (thread.joinable())
      thread.join();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... _threads joined ..." << std::endl;
}

void ThreadPool::push(RunnablePtr runnable) {
  std::lock_guard lock(_queueMutex);
  _queue.push(runnable);
  _queueCondition.notify_all();
}

RunnablePtr ThreadPool::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  RunnablePtr runnable = _queue.front();
  _queue.pop();
  return runnable;
}
