/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskController.h"
#include "ServerOptions.h"
#include "StrategySelector.h"
#include "Task.h"
#include "TaskProcessor.h"
#include "Utility.h"

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
  static TaskControllerPtr single;
  switch (op) {
  case TaskControllerOps::FETCH:
    break;
  case TaskControllerOps::CREATE:
    assert(!single && "must be destroyed or not yet created");
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
    _processors.emplace_back(processor->weak_from_this());
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
    TaskPtr task = std::make_shared<Task>(header, input);
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

bool TaskController::start(ServerOptions& options) {
  TaskControllerPtr taskController = instance(&options, TaskControllerOps::CREATE);
  return taskController->_strategy.start(options);
}

void TaskController::stopInstance() {
  // stop acceptors
  _strategy.stop();
  // stop threads
  for (auto weakPtr : _processors) {
    auto processor = weakPtr.lock();
    if (processor)
      processor->stop();
  }
  wakeupThreads();
  _threadPool.stop();
  // destroy controller
  instance(nullptr, TaskControllerOps::DESTROY);
}

void TaskController::stop() {
  instance()->stopInstance();
}

void TaskController::wakeupThreads() {
  push(std::make_shared<Task>());
}

std::atomic<unsigned>& TaskController::totalSessions() {
  return instance()->_totalSessions;
}
