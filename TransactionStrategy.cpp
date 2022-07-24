/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TransactionStrategy.h"
#include "Ad.h"
#include "ServerOptions.h"
#include "Task.h"
#include "Transaction.h"

TransactionStrategy::~TransactionStrategy() {}

void TransactionStrategy::create(const ServerOptions& options) {
  if (!Ad::load(options._adsFileName))
    return;
  Task::setPreprocessMethod(Transaction::normalizeSizeKey);
  Task::setProcessMethod(Transaction::processRequest);
}

bool TransactionStrategy::start(const ServerOptions& options) {
  return Strategy::start(options);
}

void TransactionStrategy::stop() {
  Strategy::stop();
}
