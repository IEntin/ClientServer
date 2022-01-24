/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskThread.h"
#include "ProgramOptions.h"
#include <cassert>
#include <iostream>

namespace {

volatile std::atomic<bool> stopFlag = false;

unsigned getNumberTaskThreads() {
  int numberTaskThreadsConfig = ProgramOptions::get("NumberTaskThreads", -1);
  return numberTaskThreadsConfig > 0 ? numberTaskThreadsConfig : std::thread::hardware_concurrency();
}

} // end of anonimous namespace

const bool TaskThread::_useStringView = ProgramOptions::get("StringTypeInTask", std::string()) == "STRINGVIEW";
TaskPtrSV TaskThread::_taskSV(_useStringView ? std::make_shared<TaskSV>() : TaskPtrSV());
TaskPtrST TaskThread::_taskST(_useStringView ? TaskPtrST() : std::make_shared<TaskST>());
unsigned TaskThread::_numberTaskThreads = getNumberTaskThreads();
std::barrier<CompletionFunction> TaskThread::_barrier(_numberTaskThreads, onTaskFinish);
std::vector<std::thread> TaskThread::_taskThreads;

template<typename T>
void finishTask(T& task) {
  task->finish();
  // Call static method get() through an instance of class:
  std::atomic_store(&task, task->get());
}

// Completion action is run by only one (any) blocked thread.
// Here the "first" thread is selected.

void TaskThread::onTaskFinish() noexcept {
  if (std::this_thread::get_id() == _taskThreads.front().get_id()) {
    if (_useStringView)
      finishTask(_taskSV);
    else
      finishTask(_taskST);
  }
}

// Process current task (batch of requests) by all threads.
// Only one task is processed at any given time.

template<typename T>
void TaskThread::processTask(T& task, ProcessRequest processRequest) {
  while (!stopFlag) {
    auto [view, atEnd, index, diagnostics] = task->next();
    if (!atEnd) {
      task->updateResponse(index, processRequest(view, diagnostics));
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
  for (unsigned i = 0; i < _numberTaskThreads; ++i)
    _taskThreads.emplace_back([processRequest] () {
				if (_useStringView)
				  processTask(_taskSV, processRequest);
				else
				  processTask(_taskST, processRequest); });
  return true;
}

void TaskThread::joinThreads() {
  stopFlag.store(true);
  if (_useStringView)
    TaskSV::push(std::make_shared<TaskSV>());
  else
    TaskST::push(std::make_shared<TaskST>());
  for (auto& thread : _taskThreads)
    if (thread.joinable())
      thread.join();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... taskThreads joined ..." << std::endl;
  // silence valgrind:
  std::vector<std::thread>().swap(_taskThreads);
}
