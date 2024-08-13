/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"

#include <boost/core/demangle.hpp>

std::string Runnable::getType() const {
  const auto& refObject = *this;
  return boost::core::demangle(typeid(refObject).name());
}
