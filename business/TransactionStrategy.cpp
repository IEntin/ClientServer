/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TransactionStrategy.h"

#include "Ad.h"
#include "Task.h"
#include "ServerOptions.h"
#include "Transaction.h"

void TransactionStrategy::set() {
  Ad::load(ServerOptions::_adsFileName);
  ProcessRequest function;
  if (ServerOptions::_sortInput) {
    Task::setPreprocessFunction(Transaction::createSizeKey);
    function = Transaction::processRequestSort;
    Task::setProcessFunction(function);
  }
  else {
    Task::setPreprocessFunction(nullptr);
    function = Transaction::processRequestNoSort;
    Task::setProcessFunction(function);
  }
}

TransactionStrategy::~TransactionStrategy() {
  Ad::clear();
}
