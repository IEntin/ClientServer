/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Strategy.h"

class TransactionStrategy : public Strategy {
 public:
  TransactionStrategy(const ServerOptions& options);
  
  ~TransactionStrategy() override {}

 protected:

   void create(const ServerOptions& options) override;
};
