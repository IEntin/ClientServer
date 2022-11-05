/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskProcessor.h"
#include "TaskController.h"

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
