/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskController.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "Task.h"

TaskControllerPtr TaskController::_single;
TaskController::Phase TaskController::_phase = PREPROCESSTASK;

TaskController::TaskController() :
  _barrier(ServerOptions::_numberWorkThreads, onTaskCompletion),
  _threadPool(ServerOptions::_numberWorkThreads) {
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
  auto ptr = _single;
  if (ptr)
    ptr->onCompletion();
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
    auto worker = std::make_shared<Worker>(_single);
    _threadPool.push(worker);
  }
  return true;
}

void TaskController::push(TaskPtr task) {
  std::lock_guard lock(_queueMutex);
  _queue.push(task);
  _queueCondition.notify_one();
}

void TaskController::processTask(const HEADER& header, std::string_view input, Response& response) {
  TaskPtr task = std::make_shared<Task>(response);
  task->set(header, input, response);
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
  _single = std::make_shared<TaskController>();
  return _single->start();
}

void  TaskController::stop() {
  // stop threads
  {
    std::unique_lock lock(_queueMutex);
    _stopped.store(true);
    _queueCondition.notify_one();
  }
  _threadPool.stop();
}

void TaskController::destroy() {
  if (_single)
    _single->stop();
  // destroy controller
  TaskControllerPtr().swap(_single);
}

TaskController::Worker::Worker(TaskControllerWeakPtr taskController) :
  Runnable(ServerOptions::_numberWorkThreads),
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
    if (_phase == PREPROCESSTASK) {
      while (task->preprocessNext());
      barrier.arrive_and_wait();
    }
    else if (_phase == PROCESSTASK) {
      while (task->processNext());
      barrier.arrive_and_wait();
    }
  }
}
