/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPool.h"
#include "Runnable.h"
#include "Utility.h"

ThreadPool::ThreadPool(int numberThreads) {
  start(numberThreads);
}

ThreadPool::~ThreadPool() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void ThreadPool::start(int numberThreads) {
  for (int i = 0; i < numberThreads; ++i) {
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

void ThreadPool::stop() {
  // wake up and join threads
  for (int i = 0; i < static_cast<int>(_threads.size()); ++i)
    push(RunnablePtr());
  for (auto& thread : _threads)
    if (thread.joinable())
      thread.join();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
       << " ... _threads joined ..." << std::endl;
}

void ThreadPool::push(RunnablePtr runnable) {
  std::lock_guard lock(_queueMutex);
  _queue.push(std::move(runnable));
  _queueCondition.notify_all();
}

RunnablePtr ThreadPool::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  RunnablePtr runnable = std::move(_queue.front());
  if (runnable)
    runnable->setRunning();
  _queue.pop();
  return runnable;
}
