/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "StrategySelector.h"
#include "ServerOptions.h"
#include "Logger.h"

TransactionStrategy StrategySelector::_transactionStrategy;

EchoStrategy StrategySelector::_echoStrategy;

Strategy& StrategySelector::get(const ServerOptions& options) {
  if (options._processType == "Transaction")
    return _transactionStrategy;
  else if (options._processType == "Echo")
    return _echoStrategy;
  else {
    Logger() << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << ":Strategy is not specified. Setting Transaction strategy." << std::endl; 
    return _transactionStrategy;
  }
}
