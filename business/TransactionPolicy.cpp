/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TransactionPolicy.h"

#include "Ad.h"
#include "Task.h"
#include "ServerOptions.h"
#include "Transaction.h"

void TransactionPolicy::set() {
  Ad::load(ServerOptions::_adsFileName);
  if (ServerOptions::_sortInput) {
    Task::setPreprocessFunction(Transaction::createSizeKey);
    Task::setProcessFunction(Transaction::processRequestSort);
  }
  else {
    Task::setPreprocessFunction(nullptr);
    Task::setProcessFunction(Transaction::processRequestNoSort);
  }
}
