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

std::vector<std::thread> TaskThreadPool::_taskThreads;

// parameters are desired number of work threads and
// a method to apply to every request in the task.

void TaskThreadPool::start(unsigned numberThreads, ProcessRequest processRequest) {
  TaskThread::createBarrier(numberThreads);
  for (unsigned i = 0; i < numberThreads; ++i) {
    TaskThread runnable(processRequest);
    _taskThreads.emplace_back(runnable);
  }
}

// push an empty task to the queue to
// wake up and join the threads.

void TaskThreadPool::stop() {
  stopFlag.store(true);
  Task::push(std::make_shared<Task>());
  for (auto& thread : _taskThreads)
    if (thread.joinable())
      thread.join();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... taskThreads joined ..." << std::endl;
  std::vector<std::thread>().swap(_taskThreads);
}

// start with an empty task

TaskPtr TaskThread::_task(std::make_shared<Task>());

// postpone barrier creation until desired number of threads is known.

std::unique_ptr<std::barrier<CompletionFunction>> TaskThread::_barrier;

// save function pointer to apply to every request in the task

TaskThread::TaskThread(ProcessRequest processRequest) :
  _processRequest(processRequest) {}

TaskThread::~TaskThread() {}

void TaskThread::createBarrier(unsigned numberThreads) {
  _barrier = std::make_unique<std::barrier<CompletionFunction>>(numberThreads, onTaskFinish);
}

// Process the current task (batch of requests) by all threads. Arrive
// at the sync point when the task is done and wait for the next one.

void TaskThread::operator()() noexcept {
  while (!stopFlag) {
    auto [view, atEnd, index] = _task->next();
    if (!atEnd) {
      _task->updateResponse(index, _processRequest(view));
      continue;
    }
    try {
      _barrier->arrive_and_wait();
    }
    catch (std::system_error& e) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-exception:" << e.what() << std::endl;
      std::exit(1);
    }
  }
}

// This method is called by every blocked thread when it ran out of work and waits
// for the next task. In our case only one thread is doing the actual work. This
// thread is selected arbitrarily, in this case the first created thread.

void TaskThread::onTaskFinish() noexcept {
  // static initialization happens once. It is also thread safe
  // but this is not relevant here.

  static std::thread::id firstThreadId = std::this_thread::get_id();

  if (std::this_thread::get_id() == firstThreadId) {
    _task->finish();
    // thread safe Task::get version. Blocks until the new task is available.
    Task::get(_task);

    Diagnostics::setEnabled(_task->isDiagnosticsEnabled());
  }
}
