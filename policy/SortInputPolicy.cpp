/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "SortInputPolicy.h"

#include "Ad.h"
#include "ServerOptions.h"
#include "Task.h"
#include "Transaction.h"

void SortInputPolicy::set() {
  Ad::readAds(ServerOptions::_adsFileName);
  Task::setPreprocessFunctor(Transaction::createSizeKey);
  Task::setProcessFunctor(Transaction::processRequestSort);
}
