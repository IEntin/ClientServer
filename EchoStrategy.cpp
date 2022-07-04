/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EchoStrategy.h"
#include "Echo.h"
#include "Task.h"
  
EchoStrategy::~EchoStrategy() {}

void EchoStrategy::create(const ServerOptions&) {
  Task::setProcessMethod(echo::Echo::processRequest);
}

bool EchoStrategy::start(const ServerOptions& options) {
  return Strategy::start(options);
}

void EchoStrategy::stop() {
  Strategy::stop();
}
