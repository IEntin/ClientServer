/*
 *  Copyright (C) 2021 Ilya Entin
 */
#include <boost/core/demangle.hpp>
#include "Runnable.h"
#include "Logger.h"

std::atomic<unsigned> Runnable::_numberRunningTotal = 0;

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

void Runnable::displayCapacityCheck(std::atomic<unsigned>& totalNumberObjects) const {
  Info << "Number " << getDisplayName() << " sessions=" << getNumberObjects()
       << ", Number running " << getDisplayName() << " sessions=" << getNumberRunningByType()
       << ", max number " << getDisplayName() << " running=" << _maxNumberRunningByType
       << '\n';
  switch (_status.load()) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    Warn << "\nThe number of " << getDisplayName() << " sessions="
	 << getNumberObjects() << " exceeds thread pool capacity." << '\n';
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    Warn << "\nTotal sessions=" << totalNumberObjects
	 << " exceeds system capacity." << '\n';
    break;
  default:
    break;
  }
}
