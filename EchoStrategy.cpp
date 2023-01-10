/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EchoStrategy.h"
#include "Echo.h"
#include "Task.h"

EchoStrategy::EchoStrategy(const ServerOptions& options) : Strategy(options) {}

void EchoStrategy::create(const ServerOptions&) {
  Task::setProcessMethod(Echo::processRequest);
}
