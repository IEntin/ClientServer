/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskThread.h"
#include <cassert>
#include <iostream>

extern unsigned  numberTaskThreadsCfg;

namespace {

volatile std::atomic<bool> stopFlag = false;

} // end of anonimous namespace

TaskPtr TaskThread::_task(std::make_shared<Task>());
unsigned TaskThread::_numberThreads = numberTaskThreadsCfg > 0 ? numberTaskThreadsCfg :
std::thread::hardware_concurrency();
std::barrier<CompletionFunction> TaskThread::_barrier(_numberThreads, onTaskFinish);
std::vector<std::thread> TaskThread::_taskThreads;
bool TaskThread::_diagnostics = false;

// Completion action is run by only one (any) blocked thread.
// Here the "first" thread is selected. This method is thread safe.

void TaskThread::onTaskFinish() noexcept {
  if (std::this_thread::get_id() == _taskThreads.front().get_id()) {
    _task->finish();
    _task = Task::get();
    _diagnostics = _task->isDiagnosticsEnabled();
  }
 }

// Process current task (batch of requests) by all threads.
// Only one task is processed at any given time.

void TaskThread::processTask(TaskPtr& task, ProcessRequest processRequest) {
  while (!stopFlag) {
    auto [view, atEnd, index] = task->next();
    if (!atEnd) {
      task->updateResponse(index, processRequest(view, _diagnostics));
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

bool TaskThread::startThreads(ProcessRequest processRequest) {
  for (unsigned i = 0; i < _numberThreads; ++i)
    _taskThreads.emplace_back([processRequest] () {
				processTask(_task, processRequest);
			      });
  return true;
}

void TaskThread::joinThreads() {
  stopFlag.store(true);
  Task::push(std::make_shared<Task>());
  for (auto& thread : _taskThreads)
    if (thread.joinable())
      thread.join();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... taskThreads joined ..." << std::endl;
  // silence valgrind:
  std::vector<std::thread>().swap(_taskThreads);
}
