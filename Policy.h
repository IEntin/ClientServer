/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>

class Policy {

protected:

  Policy() = default;

public:

  virtual ~Policy() {}

  virtual void set() = 0;

};
