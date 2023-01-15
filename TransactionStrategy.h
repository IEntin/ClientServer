/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Strategy.h"

class TransactionStrategy : public Strategy {
 public:
  TransactionStrategy();
  
  ~TransactionStrategy() override {}

 protected:

   void create(const ServerOptions& options) override;
};
