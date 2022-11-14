/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"

Runnable::Runnable(unsigned maxNumberThreads) : _maxNumberThreads(maxNumberThreads) {}

Runnable::~Runnable() {}

void Runnable::checkCapacity() {
  if (getNumberObjects() > _maxNumberThreads)
    _status.store(STATUS::MAX_SPECIFIC_SESSIONS);
}
