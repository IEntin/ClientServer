/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Server.h"
#include "FifoAcceptor.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "Strategy.h"
#include "StrategySelector.h"
#include "TaskController.h"
#include "TcpAcceptor.h"
#include <filesystem>
#include <fstream>

Server::Server(const ServerOptions& options) :
  _options(options),
  _threadPoolSession(_options._maxTotalSessions, &Runnable::sendStatusToClient) {
  StrategySelector strategySelector(options);
  Strategy& strategy = strategySelector.get();
  strategy.set(options);
}

bool Server::start() {
  try {
    if (!TaskController::create(_options))
      return false;
    _tcpAcceptor =
      std::make_shared<tcp::TcpAcceptor>(_options, _threadPoolAcceptor, _threadPoolSession);
    if (!_tcpAcceptor->start())
      return false;

    _fifoAcceptor =
      std::make_shared<fifo::FifoAcceptor>(_options, _threadPoolAcceptor, _threadPoolSession);
    if (!_fifoAcceptor->start())
      return false;
    std::ofstream file(_options._controlFileName);
    std::filesystem::permissions(_options._controlFileName, std::filesystem::perms::none);
  }
  catch (const std::exception& e) {
    LogError << e.what();
    return false;
  }
  return true;
}

void Server::stop() {
  if (_tcpAcceptor)
    _tcpAcceptor->stop();
  if (_fifoAcceptor)
    _fifoAcceptor->stop();
  _threadPoolAcceptor.stop();
  _threadPoolSession.stop();
  TaskController::destroy();
}
