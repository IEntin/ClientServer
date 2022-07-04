/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Strategy.h"
#include "Ad.h"
#include "FifoServer.h"
#include "Task.h"
#include "TaskController.h"
#include "ServerOptions.h"
#include "TcpServer.h"
#include "Transaction.h"
#include "Utility.h"

Strategy::~Strategy() {}

bool Strategy::start(const ServerOptions& options) {
  _fifoServer = std::make_shared<fifo::FifoServer>(options);
  if (!_fifoServer->start())
    return false;
  _tcpServer = std::make_shared<tcp::TcpServer>(options);
  if (!_tcpServer->start())
    return false;
  return true;
}

void Strategy::stop() {
  if (_tcpServer) {
    _tcpServer->stop();
    RunnablePtr().swap(_tcpServer);
  }
  if (_fifoServer) {
    _fifoServer->stop();
    RunnablePtr().swap(_fifoServer);
  }
}
