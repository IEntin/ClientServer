/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "StrategySelector.h"
#include "ServerOptions.h"
#include "Logger.h"

StrategySelector::StrategySelector(const ServerOptions& options) :
  _options(options) {}

Strategy& StrategySelector::get() {
  if (_options._processType == "Transaction")
    return _transactionStrategy;
  else if (_options._processType == "Echo")
    return _echoStrategy;
  else {
    LogError << ":Strategy is not specified. Setting Transaction strategy." << std::endl; 
    return _transactionStrategy;
  }
}
