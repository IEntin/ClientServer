/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Strategy.h"

class TransactionStrategy : public Strategy {
 public:
  TransactionStrategy() = default;
  
  ~TransactionStrategy() override;

   void onCreate(const ServerOptions& options) override;

   int onStart(const ServerOptions& options, TaskControllerPtr taskController) override;

   void onStop() override;
 private:
};
