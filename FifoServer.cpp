/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoServer.h"
#include "FifoAcceptor.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "Utility.h"

namespace fifo {

FifoServer::FifoServer(const ServerOptions& options) :
  Runnable(RunnablePtr(), TaskController::instance()),
  _options(options),
  // + 1 for acceptor
  _threadPool(_options._maxFifoConnections + 1) {
}

FifoServer::~FifoServer() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

void FifoServer::run() {}

bool FifoServer::start() {
  _acceptor = std::make_shared<FifoAcceptor>(_options, shared_from_this(), _threadPool);
  _acceptor->start();
  _threadPool.push(_acceptor);
  return true;
}

void FifoServer::stop() {
  _stopped.store(true);
  _acceptor->stop();
  RunnablePtr().swap(_acceptor);
  _threadPool.stop();
}

} // end of namespace fifo
