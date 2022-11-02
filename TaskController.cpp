/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskController.h"
#include "MemoryPool.h"
#include "ServerOptions.h"
#include "StrategySelector.h"
#include "Task.h"
#include "Utility.h"

TaskProcessor::TaskProcessor(TaskControllerPtr taskController) :
  _taskController(taskController) {}

TaskProcessor::~TaskProcessor() {}

unsigned TaskProcessor::getNumberObjects() const {
  return _objectCounter._numberObjects;
}

void TaskProcessor::run() noexcept {
  while (!_stopped) {
    _taskController->run();
  }
}

bool TaskProcessor::start() {
  return true;
}

void TaskProcessor::stop() {
  _stopped.store(true);
}

TaskController::Phase TaskController::_phase = PREPROCESSTASK;
bool TaskController::_diagnosticsEnabled = false;
std::atomic<unsigned>  TaskController::_totalSessions;

TaskController::TaskController(const ServerOptions& options) :
  _options(options),
  _sortInput(_options._sortInput),
  _barrier(_options._numberWorkThreads, onTaskCompletion),
  _threadPool(_options._numberWorkThreads),
  _strategy(StrategySelector::get(_options)) {
  MemoryPool::setExpectedSize(_options._bufferSize);
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
    taskController->onCompletionPreprocess();
    _phase = PROCESSTASK;
    return;
  case PROCESSTASK:
    taskController->onCompletionProcess();
    _phase = PREPROCESSTASK;
    return;
  }
}

void  TaskController::onCompletionPreprocess() {
  if (_sortInput)
    _task->sortIndices();
  _task->resetPointer();
}

void TaskController::onCompletionProcess() {
  _task->finish();
  // Blocks until the new task is available.
  setNextTask();
}

void TaskController::initialize() {
  for (int i = 0; i < _options._numberWorkThreads; ++i) {
    auto processor = std::make_shared<TaskProcessor>(shared_from_this());
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
  _diagnosticsEnabled = _task->diagnosticsEnabled();
  _queue.pop();
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
  instance(nullptr, DESTROY);
}

void TaskController::wakeupThreads() {
  push(std::make_shared<Task>());
}
