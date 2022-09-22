/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"

Runnable::Runnable(unsigned maxNumberThreads) : _maxNumberThreads(maxNumberThreads) {}

Runnable::~Runnable() {}

PROBLEMS Runnable::checkCapacity() const {
  if (_maxNumberThreads == 0)
    return PROBLEMS::NONE;
  if (getNumberObjects() > _maxNumberThreads)
    return PROBLEMS::MAX_NUMBER_RUNNABLES;
  return PROBLEMS::NONE;
}
