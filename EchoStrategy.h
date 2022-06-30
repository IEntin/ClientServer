/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Strategy.h"

class EchoStrategy : public Strategy {

 public:
  EchoStrategy();
  
  ~EchoStrategy() override;

   void onCreate(const ServerOptions& options) override;

   int onStart(const ServerOptions& options, TaskControllerPtr taskController) override;

   void onStop() override;
};
