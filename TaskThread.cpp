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

TaskPtr TaskThreadPool::_task(std::make_shared<Task>());
std::vector<std::thread> TaskThreadPool::_taskThreads;

TaskThreadPool::TaskThreadPool(unsigned numberThreads, ProcessRequest processRequest) :
  _numberThreads(numberThreads),
  _processRequest(processRequest),
  _barrier(_numberThreads, onTaskFinish) {}

void TaskThreadPool::start() {
  for (unsigned i = 0; i < _numberThreads; ++i) {
    TaskThread runnable(shared_from_this(), _task, _processRequest, _barrier);
    _taskThreads.emplace_back(runnable);
  }
}

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

// This method is called by all blocked threads, but in
// our case the body is executed only by one, "the first".

void TaskThreadPool::onTaskFinish() noexcept {
  static std::thread::id firstThreadId = std::this_thread::get_id();
  if (std::this_thread::get_id() == firstThreadId) {
    _task->finish();
    // thread safe Task::get version
    Task::get(_task);
    Diagnostics::setEnabled(_task->isDiagnosticsEnabled());
  }
}

TaskThread::TaskThread(TaskThreadPoolPtr pool,
		       TaskPtr& task,
		       ProcessRequest processRequest,
		       std::barrier<CompletionFunction>& barrier) :
  _pool(pool), _task(task),_processRequest(processRequest), _barrier(barrier) {}

TaskThread::~TaskThread() {}

// Process current task (batch of requests) by all threads.
// Only one task is processed at any given time.

void TaskThread::operator()() noexcept {
  while (!stopFlag) {
    auto [view, atEnd, index] = _task->next();
    if (!atEnd) {
      _task->updateResponse(index, _processRequest(view));
      continue;
    }
    try {
      _barrier.arrive_and_wait();
    }
    catch (std::system_error& e) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-exception:" << e.what() << std::endl;
      std::exit(1);
    }
  }
}
