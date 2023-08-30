/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Strategy.h"

class EchoStrategy : public Strategy {

 protected:

  void set() override;

 public:

  EchoStrategy() = default;

  ~EchoStrategy() override {}
};
