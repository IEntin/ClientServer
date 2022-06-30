/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskController.h"
#include "ServerOptions.h"
#include "StrategySelector.h"
#include "Task.h"
#include "Utility.h"
#include <cassert>

TaskController::Phase TaskController::_phase = PREPROCESSTASK;
bool TaskController::_diagnosticsEnabled = false;

TaskController::TaskController(const ServerOptions& options) :
  _options(options),
  _sortInput(_options._sortInput),
  _barrier(_options._numberWorkThreads, onTaskCompletion),
  _threadPool(_options._numberWorkThreads),
  _numberConnections(0),
  _strategy(StrategySelector::get(_options)) {
  _memoryPool.setExpectedSize(_options._bufferSize);
  // start with empty task
  _task = std::make_shared<Task>();
  _strategy.onCreate(_options);
}

TaskController::~TaskController() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

TaskControllerPtr TaskController::create(const ServerOptions& options) {
  // to have private constructor do not use make_shared
  TaskControllerPtr taskController(new TaskController(options));
  taskController->initialize();
  return taskController;
}

TaskControllerPtr TaskController::instance(const ServerOptions* options, Operations op) {
  static TaskControllerPtr instance = create(*options);
  if (op == DESTROY)
    TaskControllerPtr().swap(instance);
  return instance;
}

// This method is called by one of the threads
// when the current barrier phase is completed.

void TaskController::onTaskCompletion() noexcept {
  auto taskController = instance();
  switch (_phase) {
  case PREPROCESSTASK:
    if (taskController->_sortInput)
      taskController->_task->sortIndices();
    taskController->_task->resetPointer();
    _phase = PROCESSTASK;
    return;
  case PROCESSTASK:
    taskController->_task->finish();
    // Blocks until the new task is available.
    taskController->setNextTask();
    _phase = PREPROCESSTASK;
    return;
  }
}

void TaskController::initialize() {
  for (int i = 0; i < _options._numberWorkThreads; ++i)
    _threadPool.push(shared_from_this());
}

// Process the current task (batch of requests) by all threads. Arrive
// at the sync point when the task is done and wait for the next one.

void TaskController::run() noexcept {
  try {
    while (!_stopped) {
      while (_task->extractKeyNext());
      _barrier.arrive_and_wait();
      while (_task->processNext());
      _barrier.arrive_and_wait();
    }
  }
  catch (std::system_error& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ':' << e.what() << '\n';
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << " ! exception caught.\n";
  }
}

void TaskController::push(TaskPtr task) {
  std::lock_guard lock(_queueMutex);
  _queue.push(task);
  _queueCondition.notify_all();
}

void TaskController::submitTask(const HEADER& header, std::vector<char>& input, Response& response) {
  try {
    TaskPtr task = std::make_shared<Task>(header, input, response, _memoryPool);
    auto future = task->getPromise().get_future();
    push(task);
    future.get();
  }
  catch (std::future_error& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ':' << e.what() << '\n';
  }
}

void TaskController::setNextTask() {
  std::unique_lock lock(_queueMutex);
  _queueCondition.wait(lock, [this] { return !_queue.empty(); });
  _task = _queue.front();
  _diagnosticsEnabled = _task->diagnosticsEnabled();
  _queue.pop();
}

void TaskController::setMemoryPoolSize(size_t size) {
  _memoryPool.setExpectedSize(size);
}

int TaskController::start() {
  return _strategy.onStart(_options, shared_from_this());
}

void TaskController::stop() {
  _strategy.onStop();
  _stopped.store(true);
  wakeupThreads();
  _threadPool.stop();
  instance(nullptr, DESTROY);
}

void TaskController::wakeupThreads() {
  push(std::make_shared<Task>());
}
