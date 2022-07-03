/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Strategy.h"

class EchoStrategy : public Strategy {

 protected:

   void onCreate(const ServerOptions& options) override;

   int onStart(const ServerOptions& options) override;

   void onStop() override;

 public:

  EchoStrategy() = default;

  ~EchoStrategy() override;
};
