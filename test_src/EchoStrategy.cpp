/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EchoStrategy.h"
#include "Echo.h"
#include "Task.h"

void EchoStrategy::set() {
  ProcessRequest function = Echo::processRequest;
  Task::setPreprocessFunction(nullptr);
  Task::setProcessFunction(function);
}
