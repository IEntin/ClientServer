/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Strategy.h"
#include "FifoAcceptor.h"
#include "TcpAcceptor.h"

Strategy::~Strategy() {}

bool Strategy::start(const ServerOptions& options) {
  auto tcpAcceptor = std::make_shared<tcp::TcpAcceptor>(options);
  _tcpAcceptor = tcpAcceptor;
  if (!tcpAcceptor->start())
    return false;

  auto fifoAcceptor = std::make_shared<fifo::FifoAcceptor>(options);
  _fifoAcceptor = fifoAcceptor;
  return fifoAcceptor->start();
}

void Strategy::stop() {
  auto tcpAcceptor = _tcpAcceptor.lock();
  if (tcpAcceptor)
    tcpAcceptor->stop();
  auto fifoAcceptor = _fifoAcceptor.lock();
  if (fifoAcceptor)
    fifoAcceptor->stop();
}
