/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Strategy.h"

class TransactionStrategy : public virtual Strategy {
 public:
  TransactionStrategy();
  
  ~TransactionStrategy() override {}

 protected:

   void set(const ServerOptions& options) override;
};
