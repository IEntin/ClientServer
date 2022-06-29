/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ControllerStrategy.h"
#include "Ad.h"
#include "Echo.h"
#include "FifoServer.h"
#include "Task.h"
#include "TaskController.h"
#include "ServerOptions.h"
#include "TcpServer.h"
#include "Transaction.h"
#include "Utility.h"

void ControllerStrategy::onCreate(const ServerOptions& options) {
  if (options._processType == "Transaction") {
    if (!Ad::load(options._adsFileName))
      std::exit(4);
    Task::setPreprocessMethod(Transaction::normalizeSizeKey);
    Task::setProcessMethod(Transaction::processRequest);
  }
  else if (options._processType == "Echo")
    Task::setProcessMethod(echo::Echo::processRequest);
  else {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ":process or preprocess methods not specified.\n"; 
    std::exit(1);
  }
}

int ControllerStrategy::onStart(const ServerOptions& options, TaskControllerPtr taskController) {
  _tcpServer = std::make_shared<tcp::TcpServer>(options, taskController);
  if (!_tcpServer->start())
    return 2;
  _fifoServer = std::make_shared<fifo::FifoServer>(options, taskController);
  if (!_fifoServer->start(options))
    return 3;
  return 0;
}

void ControllerStrategy::onStop() {
  if (_tcpServer) {
    _tcpServer->stop();
    tcp::TcpServerPtr().swap(_tcpServer);
  }
  if (_fifoServer) {
    _fifoServer->stop();
    fifo::FifoServerPtr().swap(_fifoServer);
  }
}
