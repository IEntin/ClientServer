/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TransactionStrategy.h"
#include "Ad.h"
#include "AdBid.h"
#include "ServerOptions.h"
#include "Task.h"
#include "Transaction.h"

void TransactionStrategy::set(const ServerOptions& options) {
  if (!Ad::load(options._adsFileName))
    return;
  Task::setPreprocessMethod(Transaction::normalizeSizeKey);
  Task::setProcessMethod(Transaction::processRequest);
}
