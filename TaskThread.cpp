/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskThread.h"
#include <cassert>
#include <iostream>

namespace {

volatile std::atomic<bool> stopFlag = false;

} // end of anonimous namespace

TaskPtr TaskThreadPool::_task(std::make_shared<Task>());
bool TaskThreadPool::_diagnostics;
std::vector<std::thread> TaskThreadPool::_taskThreads;
std::thread::id TaskThreadPool::_firstThreadId;

TaskThreadPool::TaskThreadPool(unsigned numberThreads, ProcessRequest processRequest) :
  _numberThreads(numberThreads),
  _processRequest(processRequest),
  _barrier(_numberThreads, onTaskFinish) {
}

void TaskThreadPool::start() {
  for (unsigned i = 0; i < _numberThreads; ++i) {
    TaskThreadPtr runnable = std::make_shared<TaskThread>(shared_from_this());
    auto threadFunc = [this, runnable] () {
      runnable->processTask(_task,
			    _processRequest,
			    _barrier,
			    std::ref(_diagnostics));
    };
    std::thread newThread(threadFunc);
    if (i == 0)
      _firstThreadId = newThread.get_id();
    _taskThreads.emplace_back();
    _taskThreads.back().swap(newThread);
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

// This method is called by all, but in our case
// executed only by one, "first", blocked thread.

void TaskThreadPool::onTaskFinish() noexcept {
  if (std::this_thread::get_id() == _firstThreadId) {
    _task->finish();
    _task = Task::get();
    _diagnostics = _task->isDiagnosticsEnabled();
  }
}

TaskThread::TaskThread(TaskThreadPoolPtr pool) : _pool(pool) {}

TaskThread::~TaskThread() {}

// Process current task (batch of requests) by all threads.
// Only one task is processed at any given time.

void TaskThread::processTask(TaskPtr& task,
			     ProcessRequest processRequest,
			     std::barrier<CompletionFunction>& barrier,
			     bool& diagnostics) {
  while (!stopFlag) {
    auto [view, atEnd, index] = task->next();
    if (!atEnd) {
      task->updateResponse(index, processRequest(view, diagnostics));
      continue;
    }
    try {
      barrier.arrive_and_wait();
    }
    catch (std::system_error& e) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-exception:" << e.what() << std::endl;
      std::exit(1);
    }
  }
}
