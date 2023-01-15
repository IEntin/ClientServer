/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EchoStrategy.h"
#include "Echo.h"
#include "Task.h"

EchoStrategy::EchoStrategy() {}

void EchoStrategy::set(const ServerOptions&) {
  Task::setProcessMethod(Echo::processRequest);
}
