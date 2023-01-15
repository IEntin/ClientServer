/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Strategy.h"

class EchoStrategy : public Strategy {

 protected:

  void create(const ServerOptions& options) override;

 public:

  EchoStrategy();

  ~EchoStrategy() override {}
};
