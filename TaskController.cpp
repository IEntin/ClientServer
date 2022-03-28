/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskController.h"
#include "Diagnostics.h"
#include "Task.h"
#include <cassert>
#include <iostream>

TaskController::TaskController(unsigned numberThreads,
			       ProcessRequest processRequest,
			       size_t bufferSize) :
  _numberThreads(numberThreads),
  _processRequest(processRequest),
  _barrier(numberThreads, onTaskFinish),
  _threadPool(numberThreads) {
  _memoryPool.setInitialSize(bufferSize);
  // start with empty task
  static Batch emptyBatch;
  _task = std::make_shared<Task>(emptyBatch);
}

TaskController::~TaskController() {
  _stopped.store(true);
  _threadPool.stop();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

TaskControllerPtr TaskController::create(unsigned numberThreads,
					 ProcessRequest processRequest,
					 size_t bufferSize) {
  // to have private constructor do not use make_shared
  TaskControllerPtr taskController(new TaskController(numberThreads, processRequest, bufferSize));
  taskController->initialize();
  return taskController;
}

TaskControllerPtr TaskController::instance(unsigned numberThreads,
					   ProcessRequest processRequest,
					   size_t bufferSize) {
  static TaskControllerPtr instance = create(numberThreads, processRequest, bufferSize);
  return instance;
}

// This method is called by one of the blocked threads when
// they ran out of work and wait for the next task.

void TaskController::onTaskFinish() noexcept {
  TaskControllerPtr taskController = instance();
  taskController->_task->finish();
  // Blocks until the new task is available.
  taskController->setNextTask();
}

void TaskController::initialize() {
  for (unsigned i = 0; i < _numberThreads; ++i)
    _threadPool.push(shared_from_this());
}

// Process the current task (batch of requests) by all threads. Arrive
// at the sync point when the task is done and wait for the next one.

void TaskController::run() noexcept {
  try {
    while (!stopped()) {
      auto [request, atEnd, index] = next();
      if (!atEnd) {
	std::string response = _processRequest(request);
	updateResponse(index, response);
	continue;
      }
      _barrier.arrive_and_wait();
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

void TaskController::setNextTask() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  _task = _queue.front();
  Diagnostics::enable(_task->diagnosticsEnabled());
  _queue.pop();
}

void TaskController::setMemoryPoolSize(size_t size) {
  _memoryPool.setInitialSize(size);
}

std::tuple<std::string_view, bool, size_t> TaskController::next() {
  return _task->next();
}

void TaskController::updateResponse(size_t index, std::string& rsp) {
  _task->updateResponse(index, rsp);
}
