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
std::vector<std::pair<TaskThreadPtr, std::thread>> TaskThreadPool::_taskThreads;

TaskThreadPool::TaskThreadPool(unsigned numberThreads, ProcessRequest processRequest) :
  _numberThreads(numberThreads),
  _processRequest(processRequest),
  _barrier(_numberThreads, onTaskFinish) {
}

void TaskThreadPool::start() {
  for (unsigned i = 0; i < _numberThreads; ++i) {
    TaskThreadPtr taskThread = std::make_shared<TaskThread>(shared_from_this());
    _taskThreads.emplace_back(taskThread, [this, taskThread] () mutable {
				taskThread->processTask(_task,
							_processRequest,
							_barrier,
							std::ref(_diagnostics));
			      });
  }
}

void TaskThreadPool::stop() {
  stopFlag.store(true);
  Task::push(std::make_shared<Task>());
  for (auto& [taskThreadPtr, thread] : _taskThreads)
    if (thread.joinable())
      thread.join();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... taskThreads joined ..." << std::endl;
  std::vector<std::pair<TaskThreadPtr, std::thread>>().swap(_taskThreads);
}

// Process current task (batch of requests) by all threads.
// Only one task is processed at any given time.

void TaskThreadPool::onTaskFinish() noexcept {
  if (std::this_thread::get_id() == _taskThreads.front().second.get_id()) {
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
