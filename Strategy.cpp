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

int Strategy::onStart(const ServerOptions& options, TaskControllerPtr taskController) {
  _tcpServer = std::make_shared<tcp::TcpServer>(options, taskController);
  if (!_tcpServer->start())
    return 2;
  _fifoServer = std::make_shared<fifo::FifoServer>(options, taskController);
  if (!_fifoServer->start(options))
    return 3;
  return 0;
}

void Strategy::onStop() {
  if (_tcpServer) {
    _tcpServer->stop();
    tcp::TcpServerPtr().swap(_tcpServer);
  }
  if (_fifoServer) {
    _fifoServer->stop();
    fifo::FifoServerPtr().swap(_fifoServer);
  }
}
