/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "EchoStrategy.h"
#include "TransactionStrategy.h"

struct ServerOptions;

class StrategySelector {

 public:

  StrategySelector(const ServerOptions& options);

  ~StrategySelector() {}

  Strategy& get();

 private:
  const ServerOptions& _options;

  EchoStrategy _echoStrategy;

  TransactionStrategy _transactionStrategy;

};
