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

} // end of anonimous namespace

unsigned TaskRunnable::_numberTaskThreads = getNumberTaskThreads();
std::barrier<CompletionFunction> TaskRunnable::_barrier(_numberTaskThreads, onTaskFinish);
std::vector<std::thread> TaskRunnable::_taskThreads;

// Completion action is run by only one (any) blocked thread.
// Here the "first" thread is selected.

template<typename T>
void finishTask(T& task) {
  task->finish();
  std::atomic_store(&task, task->get());
}

void TaskRunnable::onTaskFinish() noexcept {
  if (std::this_thread::get_id() == _taskThreads.front().get_id()) {
    if (useStringView)
      finishTask(taskSV);
    else
      finishTask(taskST);
  }
}
// The goal is to start and finish current task by all threads.
// Only one task is processed at any given time which guarantees thread safety

template<typename T>
void processTask(T& task, ProcessRequest function, std::barrier<CompletionFunction>& barrier) {
  while (!stopFlag) {
    if (!task)
      task = task->instance();
    auto [view, atEnd, index] = task->next();
    if (!atEnd) {
      task->updateResponse(index, function(view));
      continue;
    }
    try {
      barrier.arrive_and_wait();
    }
    catch (std::system_error& e) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-exception:" << e.what() << std::endl;
    }
  }
}

void TaskRunnable::operator()(std::string (function)(std::string_view)) noexcept {
  if (useStringView)
    processTask(taskSV, function, _barrier);
  else
    processTask(taskST, function, _barrier);
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
