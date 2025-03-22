/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TransactionPolicy.h"

#include "Ad.h"
#include "ServerOptions.h"
#include "Task.h"
#include "Transaction.h"

void TransactionPolicy::set() {
  Ad::readAds(ServerOptions::_adsFileName);
  if (ServerOptions::_sortInput) {
    Task::setPreprocessFunctor(Transaction::createSizeKey);
    Task::setProcessFunctor(Transaction::processRequestSort);
  }
  else {
    Task::setPreprocessFunctor(nullptr);
    Task::setProcessFunctor(Transaction::processRequestNoSort);
  }
}
