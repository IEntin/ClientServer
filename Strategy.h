/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>

using StrategyPtr = std::unique_ptr<class Strategy>;

class Strategy {

protected:

  Strategy() = default;

public:

  virtual ~Strategy() {}

  virtual void set() = 0;

};
