/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "EchoStrategy.h"
#include "TransactionStrategy.h"

struct ServerOptions;

class StrategySelector {

 public:

  static Strategy& get(const ServerOptions& options);

 private:
  StrategySelector() = delete;

  ~StrategySelector() = delete;

  static TransactionStrategy _transactionStrategy;

  static EchoStrategy _echoStrategy;

};
