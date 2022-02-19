/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskThread.h"
#include "Diagnostics.h"
#include "Task.h"
#include <cassert>
#include <iostream>

TaskThreadPool::TaskThreadPool(unsigned numberThreads, ProcessRequest processRequest) :
  _numberThreads(numberThreads),
  _processRequest(processRequest),
  _barrier(numberThreads, onTaskFinish),
  _threadPool(numberThreads) {}

// This method is called for every blocked thread when it ran out of work and waits
// for the next task. In our case only one thread is doing the actual work. This
// thread is selected arbitrarily, in this case the first created thread.

void TaskThreadPool::onTaskFinish() noexcept {
  static std::thread::id firstId = std::this_thread::get_id();
  if (std::this_thread::get_id() == firstId) {
    Task::finish();
    // Blocks until the new task is available.
    Task::pop();

    Diagnostics::enable(Task::diagnosticsEnabled());
  }
}

void TaskThreadPool::start() {
  for (unsigned i = 0; i < _numberThreads; ++i) {
    TaskThreadPtr taskThread = std::make_shared<TaskThread>(shared_from_this(), _processRequest);
    taskThread->startInstance();
  }
}

// push an empty task to the queue to
// wake up and join the threads.

void TaskThreadPool::stop() {
  _stopped.store(true);
  Task::push(std::make_shared<Task>());
  _threadPool.stop();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... TaskThreadPool stopped ..." << std::endl;
}

// save pool pointer and a function pointer to apply to every request in the task
TaskThread::TaskThread(TaskThreadPoolPtr pool, ProcessRequest processRequest) :
  _pool(pool), _processRequest(processRequest) {}

TaskThread::~TaskThread() {}

void TaskThread::startInstance() {
  _pool->_threadPool.push(shared_from_this());
}

// Process the current task (batch of requests) by all threads. Arrive
// at the sync point when the task is done and wait for the next one.

void TaskThread::run() noexcept {
  try {
    while (!_pool->stopped()) {
      auto [view, atEnd, index] = Task::next();
      if (!atEnd) {
	Task::updateResponse(index, _processRequest(view));
	continue;
      }
      _pool->_barrier.arrive_and_wait();
    }
  }
  catch (std::system_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception:" << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " ! exception caught" << std::endl;
  }
}
