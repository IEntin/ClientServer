/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskController.h"
#include "ServerOptions.h"
#include "Task.h"

TaskControllerPtr TaskController::_single;

TaskController::TaskController(const ServerOptions& options) :
  _options(options),
  _sortInput(options._sortInput),
  _barrier(options._numberWorkThreads, onTaskCompletion) {
  // start with empty task
  _task = std::make_shared<Task>();
}

TaskController::~TaskController() {
  Trace << '\n';
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

void TaskController::processTask(const HEADER& header, std::string_view input, Response& response) {
  TaskPtr task = std::make_shared<Task>(header, input);
  auto future = task->getPromise().get_future();
  push(task);
  future.get();
  task->getResponse(response);
}

void TaskController::setNextTask() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty() || _stopped; });
  if (_stopped)
    return;
  _task = _queue.front();
  _queue.pop();
}

bool TaskController::isDiagnosticsEnabled() {
  if (_single->_task)
    return _single->_task->diagnosticsEnabled();
  return false;
}

bool TaskController::create(const ServerOptions& options) {
  _single = std::make_shared<TaskController>(options);
  return _single->start();
}

void  TaskController::stop() {
  // stop threads
  _stopped = true;
  _queueCondition.notify_one();
  _threadPool.stop();
}

void TaskController::destroy() {
  if (_single)
    _single->stop();
  // destroy controller
  TaskControllerPtr().swap(_single);
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
    while (task->extractKeyNext());
    barrier.arrive_and_wait();
    while (task->processNext());
    barrier.arrive_and_wait();
  }
}
