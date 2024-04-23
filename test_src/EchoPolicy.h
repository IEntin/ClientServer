/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Policy.h"

class EchoPolicy : public Policy {

 protected:

  void set() override;

 public:

  EchoPolicy() = default;

  ~EchoPolicy() override {}
};
