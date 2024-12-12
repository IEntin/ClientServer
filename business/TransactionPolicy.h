/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Policy.h"

class TransactionPolicy : public Policy {
 public:
  TransactionPolicy() {}
  
  ~TransactionPolicy() override {}

 protected:

   void set() override;
};
