#include "TaskRunnable.h"
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

const bool TaskRunnable::_useStringView = ProgramOptions::get("StringTypeInTask", std::string()) == "STRINGVIEW";
TaskPtrSV TaskRunnable::_taskSV(_useStringView ? std::make_shared<TaskSV>() : TaskPtrSV());
TaskPtrST TaskRunnable::_taskST(_useStringView ? TaskPtrST() : std::make_shared<TaskST>());
unsigned TaskRunnable::_numberTaskThreads = getNumberTaskThreads();
std::barrier<CompletionFunction> TaskRunnable::_barrier(_numberTaskThreads, onTaskFinish);
std::vector<std::thread> TaskRunnable::_taskThreads;

template<typename T>
void finishTask(T& task) {
  task->finish();
  // Call static method get() through an instance of class:
  std::atomic_store(&task, task->get());
}

// Completion action is run by only one (any) blocked thread.
// Here the "first" thread is selected.

void TaskRunnable::onTaskFinish() noexcept {
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
void TaskRunnable::processTask(T& task, ProcessRequest processRequest) {
  while (!stopFlag) {
    auto [view, atEnd, index] = task->next();
    if (!atEnd) {
      task->updateResponse(index, processRequest(view));
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

void TaskRunnable::threadFunc(ProcessRequest processRequest) {
  if (_useStringView)
    processTask(_taskSV, processRequest);
  else
    processTask(_taskST, processRequest);
}

bool TaskRunnable::startThreads(ProcessRequest processRequest) {
  for (unsigned i = 0; i < _numberTaskThreads; ++i)
    _taskThreads.emplace_back(threadFunc, processRequest);
  return true;
}

void TaskRunnable::joinThreads() {
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
