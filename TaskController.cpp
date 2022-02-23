/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskController.h"
#include "Diagnostics.h"
#include "Task.h"
#include <cassert>
#include <iostream>

TaskControllerPtr TaskController::_instance;

TaskController::TaskController(unsigned numberThreads, ProcessRequest processRequest) :
  _numberThreads(numberThreads),
  _processRequest(processRequest),
  _barrier(numberThreads, onTaskFinish),
  _threadPool(numberThreads) {
  // start with empty task
  static Batch emptyBatch;
  _task = std::make_shared<Task>(emptyBatch);
}

// This method is called for every blocked thread when it ran out of work and waits
// for the next task. In our case only one thread is doing the actual work. This
// thread is selected arbitrarily, in this case the first created thread.

void TaskController::onTaskFinish() noexcept {
  static std::thread::id firstId = std::this_thread::get_id();
  if (std::this_thread::get_id() == firstId) {
    _instance->_task->finish();
    // Blocks until the new task is available.
    _instance->setNew();
  }
}

void TaskController::startInstance() {
  for (unsigned i = 0; i < _numberThreads; ++i) {
    TaskProcessorPtr taskProcessor = std::make_shared<TaskProcessor>(shared_from_this(), _processRequest);
    taskProcessor->startInstance();
  }
}

TaskControllerPtr TaskController::start(unsigned numberThreads, ProcessRequest processRequest) {
  _instance = std::make_shared<TaskController>(numberThreads, processRequest);
  _instance->startInstance();
  return _instance;
}

void TaskController::push(TaskPtr task) {
  std::lock_guard lock(_queueMutex);
  _queue.push(task);
  _queueCondition.notify_all();
}

void TaskController::submitTask(const HEADER& header, std::vector<char>& input, Batch& response) {
  try {
    TaskPtr task = std::make_shared<Task>(header, input, response);
    auto future = task->getPromise().get_future();
    push(task);
    future.get();
  }
  catch (std::future_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception:" << e.what() << std::endl;
  }
}

void TaskController::setNew() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  _task = _queue.front();
  Diagnostics::enable(_task->diagnosticsEnabled());
  _queue.pop();
}

std::tuple<std::string_view, bool, size_t> TaskController::next() {
  return _task->next();
}

void TaskController::updateResponse(size_t index, std::string& rsp) {
  _task->updateResponse(index, rsp);
}

// push an empty task to the queue to
// wake up and join the threads.
void TaskController::stopInstance() {
  _stopped.store(true);
  static Batch emptyBatch;
  push(std::make_shared<Task>(emptyBatch));
  _threadPool.stop();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << " ... TaskController stopped ..." << std::endl;
}

void TaskController::stop() {
  if (_instance)
    _instance->stopInstance();
  _instance.reset();
}

// save controller pointer and a function pointer to apply to every request in the task
TaskProcessor::TaskProcessor(TaskControllerPtr controller, ProcessRequest processRequest) :
  _controller(controller), _processRequest(processRequest) {}

TaskProcessor::~TaskProcessor() {}

void TaskProcessor::startInstance() {
  _controller->_threadPool.push(shared_from_this());
}

// Process the current task (batch of requests) by all threads. Arrive
// at the sync point when the task is done and wait for the next one.

void TaskProcessor::run() noexcept {
  try {
    while (!_controller->stopped()) {
      auto [request, atEnd, index] = _controller->next();
      if (!atEnd) {
	std::string response = _processRequest(request);
	_controller->updateResponse(index, response);
	continue;
      }
      _controller->_barrier.arrive_and_wait();
    }
  }
  catch (std::system_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception:" << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " ! exception caught" << std::endl;
  }
}
