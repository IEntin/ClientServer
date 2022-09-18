/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ThreadPool.h"
#include "Header.h"
#include "Utility.h"

std::shared_ptr<KillThread> ThreadPool::_killThread = std::make_shared<KillThread>();

ThreadPool::ThreadPool(unsigned maxNumberThreads) : _maxNumberThreads(maxNumberThreads) {
  for (unsigned i = 0; i < _maxNumberThreads; ++i)
    createThread();
}

ThreadPool::~ThreadPool() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void ThreadPool::stop() {
  // wake up and join threads
  for (unsigned i = 0; i < _threads.size(); ++i)
    push(_killThread);
  for (auto& thread : _threads)
    if (thread.joinable())
      thread.join();
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
  std::lock_guard lock(_queueMutex);
  PROBLEMS problem = PROBLEMS::NONE;
  if (runnable) {
    if (_maxNumberThreads == 0 && runnable->getNumberObjects() > _threads.size())
      createThread();
    else
      problem = runnable->checkCapacity();
  }
  _queue.push(runnable);
  _queueCondition.notify_all();
  return problem;
}

RunnablePtr ThreadPool::get() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  RunnablePtr runnable = _queue.front();
  _queue.pop();
  return runnable;
}
