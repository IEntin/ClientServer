/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EchoPolicy.h"
#include "Echo.h"
#include "Task.h"

void EchoPolicy::set() {
  Task::setPreprocessFunction(nullptr);
  Task::setProcessFunction(Echo::processRequest);
}
