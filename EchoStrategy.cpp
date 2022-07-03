/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EchoStrategy.h"
#include "Echo.h"
#include "Task.h"
  
EchoStrategy::~EchoStrategy() {}

void EchoStrategy::onCreate(const ServerOptions&) {
  Task::setProcessMethod(echo::Echo::processRequest);
}

int EchoStrategy::onStart(const ServerOptions& options) {
  return Strategy::onStart(options);
}

void EchoStrategy::onStop() {
  Strategy::onStop();
}
