/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskController.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "StrategySelector.h"
#include "Task.h"

TaskControllerPtr TaskController::_single;

TaskController::TaskController(const ServerOptions& options) :
  _options(options),
  _sortInput(_options._sortInput),
  _barrier(_options._numberWorkThreads, onTaskCompletion),
  _threadPool(_options._numberWorkThreads),
  _strategy(StrategySelector::get(_options)) {
  // start with empty task
  _task = std::make_shared<Task>();
  _strategy.create(_options);
}

TaskController::~TaskController() {
  Logger(LOG_LEVEL::TRACE) << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

TaskControllerWeakPtr TaskController::weakInstance() {
  return _single;
}

// This method is called by one of the threads
// when the current barrier phase completes.

void TaskController::onTaskCompletion() noexcept {
  TaskControllerWeakPtr weakPtr(_single);
  auto ptr = weakPtr.lock();
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
    Error() << __FILE__ << ':' << __LINE__ << ' ' << __func__
	    << ':' << e.what() << std::endl;
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

bool TaskController::create(ServerOptions& options) {
  _single = std::make_shared<TaskController>(options);
  _single->start();
  return _single->_strategy.start(options);
}

void  TaskController::stop() {
  // stop acceptors
  _strategy.stop();
  // stop threads
  _stopped.store(true);
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

std::atomic<unsigned>& TaskController::totalSessions() {
  static std::atomic<unsigned> zero;
  TaskControllerWeakPtr weakPtr = _single;
  auto ptr = weakPtr.lock();
  if (!ptr)
    return zero;
  return ptr->_totalSessions;
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
      Error() << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ':' << e.what() << std::endl;
    }
    catch (...) {
      Error() << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " ! exception caught." << std::endl;
    }
  }
}
