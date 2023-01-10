/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskController.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "Strategy.h"
#include "Task.h"

TaskControllerPtr TaskController::_single;

TaskController::TaskController(const ServerOptions& options, Strategy& strategy) :
  _options(options),
  _sortInput(options._sortInput),
  _barrier(options._numberWorkThreads, onTaskCompletion),
  _threadPool(options._numberWorkThreads),
  _strategy(strategy) {
  // start with empty task
  _task = std::make_shared<Task>();
  _strategy.create(options);
}

TaskController::~TaskController() {
  Trace << std::endl;
}

TaskControllerWeakPtr TaskController::weakInstance() {
  return _single;
}

// This method is called by one of the threads
// when the current barrier phase completes.

void TaskController::onTaskCompletion() noexcept {
  TaskControllerPtr ptr = _single;
  if (ptr)
    ptr->onCompletion();
}

void TaskController::onCompletion() {
  switch (_phase) {
  case PREPROCESSTASK:
    if (_sortInput)
      _task->sortIndices();
    _task->resetPointer();
    _phase = PROCESSTASK;
    break;
  case PROCESSTASK:
    _task->finish();
    // Blocks until the new task is available.
    setNextTask();
    _phase = PREPROCESSTASK;
    break;
  default:
    break;
  }
}

bool TaskController::start() {
  for (int i = 0; i < _options._numberWorkThreads; ++i) {
    auto worker = std::make_shared<Worker>(_single);
    _threadPool.push(worker);
  }
  return true;
}

void TaskController::push(TaskPtr task) {
  std::lock_guard lock(_queueMutex);
  _queue.push(task);
  _queueCondition.notify_all();
}

void TaskController::processTask(const HEADER& header, std::vector<char>& input, Response& response) {
  try {
    TaskPtr task = std::make_shared<Task>(header, input);
    auto future = task->getPromise().get_future();
    push(task);
    future.get();
    task->getResponse(response);
  }
  catch (std::future_error& e) {
    LogError << e.what() << std::endl;
  }
}

void TaskController::setNextTask() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  _task = _queue.front();
  _queue.pop();
}

bool TaskController::isDiagnosticsEnabled() {
  if (_single->_task)
    return _single->_task->diagnosticsEnabled();
  return false;
}

bool TaskController::create(ServerOptions& options, Strategy& strategy) {
  _single = std::make_shared<TaskController>(options, strategy);
  _single->start();
  return _single->_strategy.start();
}

void  TaskController::stop() {
  // stop acceptors
  _strategy.stop();
  // stop threads
  _stopped = true;
  wakeupThreads();
  _threadPool.stop();
}

void TaskController::destroy() {
  if (_single)
    _single->stop();
  // destroy controller
  TaskControllerPtr().swap(_single);
}

void TaskController::wakeupThreads() {
  push(std::make_shared<Task>());
}

TaskController::Worker::Worker(TaskControllerWeakPtr taskController) :
  _taskController(taskController) {}

// Process the current task (batch of requests) by all threads. Arrive
// at the sync point when the task is done and wait for the next one.

void TaskController::Worker::run() noexcept {
  auto taskController = _taskController.lock();
  if (!taskController)
    return;
  auto& stopped = taskController->_stopped;
  auto& task = taskController->_task;
  auto& barrier = taskController->_barrier;
  while (!stopped) {
    try {
      while (task->extractKeyNext());
      barrier.arrive_and_wait();
      while (task->processNext());
      barrier.arrive_and_wait();
    }
    catch (std::exception& e) {
      LogError << e.what() << std::endl;
    }
    catch (...) {
      LogError << "! exception caught." << std::endl;
    }
  }
}
