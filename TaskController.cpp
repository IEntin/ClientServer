/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskController.h"
#include "ServerOptions.h"
#include "StrategySelector.h"
#include "Task.h"
#include "Utility.h"

TaskProcessor::TaskProcessor(TaskControllerWeakPtr taskController) :
  _taskController(taskController) {}

TaskProcessor::~TaskProcessor() {}

unsigned TaskProcessor::getNumberObjects() const {
  return _objectCounter._numberObjects;
}

void TaskProcessor::run() noexcept {
  while (!_stopped) {
    auto taskController = _taskController.lock();
    if (taskController)
      taskController->run();
  }
}

bool TaskProcessor::start() {
  return true;
}

void TaskProcessor::stop() {
  _stopped.store(true);
}

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
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

TaskControllerPtr TaskController::create(const ServerOptions& options) {
  // to have private constructor do not use make_shared
  TaskControllerPtr taskController(new TaskController(options));
  taskController->initialize();
  return taskController;
}

TaskControllerPtr TaskController::instance(const ServerOptions* options, TaskControllerOps op) {
  static TaskControllerPtr single = create(*options);
  switch (op) {
  case TaskControllerOps::KEEP:
    break;
  case TaskControllerOps::RECREATE:
    if (!single)
      single = create(*options);
    break;
  case TaskControllerOps::DESTROY:
    TaskControllerPtr().swap(single);
    break;
  default:
    break;
  }
  return single;
}

// This method is called by one of the threads
// when the current barrier phase is completed.

void TaskController::onTaskCompletion() noexcept {
  auto taskController = instance();
  taskController->onCompletion();
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

void TaskController::initialize() {
  for (int i = 0; i < _options._numberWorkThreads; ++i) {
    auto processor = std::make_shared<TaskProcessor>(weak_from_this());
    _processors.emplace_back(processor);
    _threadPool.push(processor);
  }
}

// Process the current task (batch of requests) by all threads. Arrive
// at the sync point when the task is done and wait for the next one.

void TaskController::run() noexcept {
  try {
    while (_task->extractKeyNext());
    _barrier.arrive_and_wait();
    while (_task->processNext());
    _barrier.arrive_and_wait();
  }
  catch (std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << std::endl;
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " ! exception caught." << std::endl;
  }
}

void TaskController::push(TaskPtr task) {
  std::lock_guard lock(_queueMutex);
  _queue.push(task);
  _queueCondition.notify_all();
}

void TaskController::processTask(const HEADER& header, std::vector<char>& input, Response& response) {
  try {
    TaskPtr task = std::make_shared<Task>(header, input, response);
    auto future = task->getPromise().get_future();
    push(task);
    future.get();
    task->getResponse(response);
  }
  catch (std::future_error& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << std::endl;
  }
}

void TaskController::setNextTask() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  _task = _queue.front();
  _queue.pop();
}

bool TaskController::isDiagnosticsEnabled() {
  if (instance()->_task)
    return instance()->_task->diagnosticsEnabled();
  return false;
}

bool TaskController::start() {
  return _strategy.start(_options);
}

void TaskController::stop() {
  // stop acceptors
  _strategy.stop();
  // stop threads
  for (auto processor : _processors)
    processor->stop();
  wakeupThreads();
  _threadPool.stop();
  // destroy controller
  instance(nullptr, TaskControllerOps::DESTROY);
}

void TaskController::wakeupThreads() {
  push(std::make_shared<Task>());
}

std::atomic<unsigned>& TaskController::totalSessions() {
  return instance()->_totalSessions;
}
