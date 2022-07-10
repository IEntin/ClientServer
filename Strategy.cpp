/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Strategy.h"
#include "FifoAcceptor.h"
#include "ServerOptions.h"
#include "TcpServer.h"

Strategy::~Strategy() {}

bool Strategy::start(const ServerOptions& options) {
  _fifoAcceptor = std::make_shared<fifo::FifoAcceptor>(options);
  if (!_fifoAcceptor->start())
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
  if (_fifoAcceptor) {
    _fifoAcceptor->stop();
    RunnablePtr().swap(_fifoAcceptor);
  }
}
