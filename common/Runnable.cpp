/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"

#include <boost/core/demangle.hpp>

bool Runnable::checkCapacity() {
  if (getNumberObjects() > _maxNumberRunningByType) {
    _status = STATUS::MAX_OBJECTS_OF_TYPE;
    return false;
  }
  return true;
}

std::string Runnable::getType() const {
  const auto& refObject = *this;
  return boost::core::demangle(typeid(refObject).name());
}
