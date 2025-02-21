/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskController.h"

#include "Logger.h"
#include "ServerOptions.h"
#include "Task.h"

TaskControllerPtr TaskController::_instance;
TaskController::Phase TaskController::_phase = PREPROCESSTASK;
std::mutex TaskController::_mutex;

TaskController::TaskController() :
  _barrier(ServerOptions::_numberWorkThreads, onTaskCompletion),
  _threadPool(ServerOptions::_numberWorkThreads) {
  // start with empty task
  _task = std::make_shared<Task>();
}

TaskControllerWeakPtr TaskController::getWeakPtr() {
  return _instance->weak_from_this();
}

// This method is called by one of the threads
// when the current barrier phase completes.

void TaskController::onTaskCompletion() noexcept {
  if (_instance)
    _instance->onCompletion();
}

void TaskController::onCompletion() {
  switch (_phase) {
  case PREPROCESSTASK:
    if (ServerOptions::_sortInput)
      _task->sortIndices();
    _task->resetIndex();
    _phase = PROCESSTASK;
    break;
  case PROCESSTASK:
    _task->finish();
    // Blocks until the new task is available.
    setNextTask();
    _task->resetIndex();
    _phase = PREPROCESSTASK;
    break;
  default:
    break;
  }
}

bool TaskController::start() {
  for (int i = 0; i < ServerOptions::_numberWorkThreads; ++i) {
    auto worker = std::make_shared<Worker>(_instance);
    _threadPool.push(worker);
  }
  return true;
}

void TaskController::push(TaskPtr task) {
  std::lock_guard lock(_queueMutex);
  _queue.push(task);
  _queueCondition.notify_one();
}

void TaskController::processTask(TaskPtr task) {
  auto future = task->getPromise().get_future();
  push(task);
  future.get();
}

void TaskController::setNextTask() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty() || _stopped; });
  if (_stopped)
    return;
  _task = _queue.front();
  _queue.pop();
}

bool TaskController::create() {
  std::lock_guard lock(_mutex);
  if (!_instance)
    _instance = std::make_shared<TaskController>();
  return _instance->start();
}

void  TaskController::stop() {
  // stop threads
  {
    std::lock_guard lock(_queueMutex);
    _stopped.store(true);
    _queueCondition.notify_one();
  }
  _threadPool.stop();
}

void TaskController::destroy() {
  if (_instance)
    _instance->stop();
  // destroy controller
  std::shared_ptr<TaskController>().swap(_instance);
}

TaskController::Worker::Worker(TaskControllerWeakPtr taskController) :
  Runnable(ServerOptions::_numberWorkThreads),
  _taskController(taskController) {}

// Process the current task (batch of requests) by all threads. Arrive
// at the sync point when the task is done and wait for the next one.

void TaskController::Worker::run() noexcept {
  if (auto taskController = _taskController.lock(); taskController) {
    auto& stopped = taskController->_stopped;
    auto& task = taskController->_task;
    auto& barrier = taskController->_barrier;
    while (!stopped) {
      if (_phase == PROCESSTASK) {
	while (task->processNext());
	barrier.arrive_and_wait();
      }
      else if (_phase == PREPROCESSTASK) {
	if (Task::_preprocessRequest)
	  while (task->preprocessNext());
	barrier.arrive_and_wait();
      }
    }
  }
}
