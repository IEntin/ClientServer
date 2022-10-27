/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"

Runnable::Runnable(unsigned maxNumberThreads) : _maxNumberThreads(maxNumberThreads) {}

Runnable::~Runnable() {}

void Runnable::checkCapacity() {
  if (getNumberObjects() > _maxNumberThreads)
    _problem.store(PROBLEMS::MAX_NUMBER_RUNNABLES);
}
