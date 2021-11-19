#include "TaskRunnable.h"
#include "ProgramOptions.h"
#include "TaskTemplate.h"
#include <cassert>
#include <iostream>

namespace {

volatile std::atomic<bool> stopFlag = false;

unsigned getNumberTaskThreads() {
  int numberTaskThreadsConfig = ProgramOptions::get("NumberTaskThreads", -1);
  return numberTaskThreadsConfig > 0 ? numberTaskThreadsConfig : std::thread::hardware_concurrency();
}

const bool useStringView = ProgramOptions::get("StringTypeInTask", std::string()) == "STRINGVIEW";
TaskPtrSV taskSV;
TaskPtrST taskST;

bool createInstance() {
  if (useStringView)
    taskSV = TaskSV::instance();
  else
    taskST = TaskST::instance();
  return true;
}

bool dummy[[maybe_unused]] = createInstance();

} // end of anonimous namespace

unsigned TaskRunnable::_numberTaskThreads = getNumberTaskThreads();
std::barrier<CompletionFunction> TaskRunnable::_barrier(_numberTaskThreads, onTaskFinish);
std::vector<std::thread> TaskRunnable::_taskThreads;

// Completion action is run by only one (any) blocked thread.
// Here the "first" thread is selected.
void TaskRunnable::onTaskFinish() noexcept {
  if (std::this_thread::get_id() == _taskThreads.front().get_id()) {
    if (useStringView) {
      taskSV->finish();
      std::atomic_store(&taskSV, TaskSV::get());
    }
    else {
      taskST->finish();
      std::atomic_store(&taskST, TaskST::get());
    }
  }
}
// The goal is to start and finish current task by all threads.
// Only one task is processed at any given time which guarantees thread safety
void TaskRunnable::operator()(std::string (function)(std::string_view)) noexcept {
  if (useStringView) {
    while (!stopFlag) {
      auto [view, atEnd, index] = taskSV->next();
      if (!atEnd) {
	taskSV->updateResponse(index, function(view));
	continue;
      }
      try {
	_barrier.arrive_and_wait();
      }
      catch (std::system_error& e) {
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		  << "-exception:" << e.what() << std::endl;
      }
    }
  }
  else {
    while (!stopFlag) {
      auto [view, atEnd, index] = taskST->next();
      if (!atEnd) {
	taskST->updateResponse(index, function(view));
	continue;
      }
      try {
	_barrier.arrive_and_wait();
      }
      catch (std::system_error& e) {
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		  << "-exception:" << e.what() << std::endl;
      }
    }
  }
}

bool TaskRunnable::startThreads(ProcessRequest processRequest) {
  for (unsigned i = 0; i < _numberTaskThreads; ++i)
    _taskThreads.emplace_back(TaskRunnable(), processRequest);
  return true;
}

void TaskRunnable::joinThreads() {
  stopFlag.store(true);
  if (useStringView)
    TaskSV::push(std::make_shared<TaskSV>());
  else
    TaskST::push(std::make_shared<TaskST>());
  for (auto& thread : _taskThreads)
    if (thread.joinable())
      thread.join();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... taskThreads joined ..." << std::endl;
  // silence valgrind false positives.
  std::vector<std::thread>().swap(_taskThreads);
}
