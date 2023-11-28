/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EchoStrategy.h"
#include "Echo.h"
#include "Task.h"

void EchoStrategy::set() {
  Task::setPreprocessFunction(nullptr);
  Task::setProcessFunction(Echo::processRequest);
}
