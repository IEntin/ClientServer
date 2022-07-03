/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TransactionStrategy.h"
#include "Ad.h"
#include "FifoServer.h"
#include "Task.h"
#include "TaskController.h"
#include "ServerOptions.h"
#include "TcpServer.h"
#include "Transaction.h"
#include "Utility.h"

TransactionStrategy::~TransactionStrategy() {}

void TransactionStrategy::onCreate(const ServerOptions& options) {
  if (!Ad::load(options._adsFileName))
    std::exit(4);
  Task::setPreprocessMethod(Transaction::normalizeSizeKey);
  Task::setProcessMethod(Transaction::processRequest);
}

int TransactionStrategy::onStart(const ServerOptions& options) {
  return Strategy::onStart(options);
}

void TransactionStrategy::onStop() {
  Strategy::onStop();
}
