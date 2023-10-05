/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Strategy.h"

class TransactionStrategy : public Strategy {
 public:
  TransactionStrategy() = default;
  
  ~TransactionStrategy() override;

 protected:

   void set() override;
};
