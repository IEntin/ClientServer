/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TransactionStrategy.h"
#include "Ad.h"
#include "Task.h"
#include "ServerOptions.h"
#include "Transaction.h"

using SIZETUPLE = std::tuple<unsigned, unsigned>;

void TransactionStrategy::set() {
  Ad::load(ServerOptions::_adsFileName);
  Task::setPreprocessMethod(Transaction::createSizeKey);
  Task::setProcessMethod(Transaction::processRequest);
}

TransactionStrategy::~TransactionStrategy() {
  Ad::clear();
}
