/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Strategy.h"
#include "FifoAcceptor.h"
#include "ServerOptions.h"
#include "SessionContainer.h"
#include "TcpAcceptor.h"

Strategy::Strategy(const ServerOptions& options) :
  _options(options), _sessionContainer(options) {}

bool Strategy::start() {
  RunnablePtr tcpAcceptor = std::make_shared<tcp::TcpAcceptor>(_options, _sessionContainer);
  _tcpAcceptor = tcpAcceptor;
  if (!tcpAcceptor->start())
    return false;

  RunnablePtr fifoAcceptor = std::make_shared<fifo::FifoAcceptor>(_options, _sessionContainer);
  _fifoAcceptor = fifoAcceptor;
  return fifoAcceptor->start();
}

void Strategy::stop() {
  RunnablePtr tcpAcceptor = _tcpAcceptor.lock();
  if (tcpAcceptor)
    tcpAcceptor->stop();
  RunnablePtr fifoAcceptor = _fifoAcceptor.lock();
  if (fifoAcceptor)
    fifoAcceptor->stop();
}
