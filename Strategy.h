/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>

using StrategyPtr = std::shared_ptr<class Strategy>;

class Strategy {

 public:

  Strategy() = default;

  virtual ~Strategy() {};

  virtual void set() = 0;

};
