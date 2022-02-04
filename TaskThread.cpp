/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskThread.h"
#include "Diagnostics.h"
#include <cassert>
#include <iostream>

namespace {

volatile std::atomic<bool> stopFlag = false;

} // end of anonimous namespace

TaskThreadPool::TaskThreadPool(unsigned numberThreads, ProcessRequest processRequest) :
  _numberThreads(numberThreads),
  _processRequest(processRequest),
  _barrier(numberThreads, TaskThreadPool::onTaskFinish) {}

// This method is called for every blocked thread when it ran out of work and waits
// for the next task. In our case only one thread is doing the actual work. This
// thread is selected arbitrarily, in this case the first created thread.

void TaskThreadPool::onTaskFinish() noexcept {
  // static initialization happens once. It is also thread safe (not relevant here).
  static std::thread::id firstThreadId = std::this_thread::get_id();

  if (std::this_thread::get_id() == firstThreadId) {
    Task::finish();
    // Blocks until the new task is available.
    Task::pop();

    Diagnostics::enable(Task::diagnosticsEnabled());
  }
}

void TaskThreadPool::start() {
  for (unsigned i = 0; i < _numberThreads; ++i)
    _threads.emplace_back(std::make_shared<TaskThread>(shared_from_this(), _processRequest));
}

// push an empty task to the queue to
// wake up and join the threads.

void TaskThreadPool::stop() {
  stopFlag.store(true);
  Task::push(std::make_shared<Task>());
  std::vector<TaskThreadPtr>().swap(_threads);
  std::clog << "... TaskThreadPool stopped ..." << std::endl;
}

// save pool pointer and a function pointer to apply to every request in the task
TaskThread::TaskThread(TaskThreadPoolPtr pool, ProcessRequest processRequest) :
  _runnable(pool, processRequest), _thread(_runnable) {}

TaskThread::~TaskThread() {
  if (_thread.joinable())
    _thread.join();
}

TaskThread::Runnable::Runnable(TaskThreadPoolPtr pool, ProcessRequest processRequest) :
  _pool(pool), _barrier(pool->_barrier), _processRequest(processRequest) {}

// Process the current task (batch of requests) by all threads. Arrive
// at the sync point when the task is done and wait for the next one.

void TaskThread::Runnable::operator()() noexcept {
  while (!stopFlag) {
    auto [view, atEnd, index] = Task::next();
    if (!atEnd) {
      Task::updateResponse(index, _processRequest(view));
      continue;
    }
    try {
      _barrier.get().arrive_and_wait();
    }
    catch (std::system_error& e) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-exception:" << e.what() << std::endl;
      std::exit(1);
    }
  }
}
