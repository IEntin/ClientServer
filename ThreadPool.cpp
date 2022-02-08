#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(unsigned numberThreads) {
  for (unsigned i = 0; i < numberThreads; ++i) {
    _threads.emplace_back([this] () {
			    while (true) {
			      {
				std::shared_ptr runnable = get();
				if (!runnable)
				  break;
				runnable->run();
			      }
			    }
			  });
  }
}

ThreadPool::~ThreadPool() {
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
  /*
  for (unsigned i = 0; i < _threads.size(); ++i)
    push(std::shared_ptr<Runnable>());
  for (auto& thread : _threads)
    if (thread.joinable()) {
      thread.join();
  }
  */
}

void ThreadPool::stop() {
  for (unsigned i = 0; i < _threads.size(); ++i)
    push(std::shared_ptr<Runnable>());
  for (auto& thread : _threads)
    if (thread.joinable())
      thread.join();
}

void ThreadPool::push(RunnablePtr runnable) {
  std::lock_guard lock(_queueMutex);
  _queue.push(runnable);
  _queueCondition.notify_all();
}

std::shared_ptr<Runnable> ThreadPool::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  RunnablePtr runnable = _queue.front();
  _queue.pop();
  return runnable;
}
