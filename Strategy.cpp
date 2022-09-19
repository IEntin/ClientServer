/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Strategy.h"
#include "FifoAcceptor.h"
#include "ServerOptions.h"
#include "TcpAcceptor.h"

Strategy::~Strategy() {}

bool Strategy::start(const ServerOptions& options) {
  _tcpAcceptor = std::make_shared<tcp::TcpAcceptor>(options);
  if (!_tcpAcceptor->start())
    return false;
  _fifoAcceptor = std::make_shared<fifo::FifoAcceptor>(options);
  if (!_fifoAcceptor->start())
    return false;
  return true;
}

void Strategy::stop() {
  if (_tcpAcceptor)
    _tcpAcceptor->stop();
  if (_fifoAcceptor)
    _fifoAcceptor->stop();
}
