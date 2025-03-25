/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "NoSortInputPolicy.h"

#include "Ad.h"
#include "ServerOptions.h"
#include "Task.h"
#include "Transaction.h"

void NoSortInputPolicy::set() {
  Ad::readAds(ServerOptions::_adsFileName);
  Task::setPreprocessFunctor(nullptr);
  Task::setProcessFunctor(Transaction::processRequestNoSort);
}
