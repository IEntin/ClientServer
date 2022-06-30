/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EchoStrategy.h"
#include "Echo.h"
#include "Task.h"

EchoStrategy::EchoStrategy() {}
  
EchoStrategy::~EchoStrategy() {}

void EchoStrategy::onCreate(const ServerOptions&) {
  Task::setProcessMethod(echo::Echo::processRequest);
}

int EchoStrategy::onStart(const ServerOptions& options, TaskControllerPtr taskController) {
  return Strategy::onStart(options, taskController);
}

void EchoStrategy::onStop() {
  Strategy::onStop();
}
