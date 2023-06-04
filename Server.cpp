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
#include <boost/interprocess/sync/named_mutex.hpp>
#include <filesystem>
#include <fstream>

Server::Server(const ServerOptions& options) :
  _options(options),
  _threadPoolSession(_options._maxTotalSessions, &Runnable::sendStatusToClient) {
  boost::interprocess::named_mutex::remove(FIFO_NAMED_MUTEX);
  StrategySelector strategySelector(options);
  Strategy& strategy = strategySelector.get();
  strategy.set(options);
}

Server::~Server() {
  boost::interprocess::named_mutex::remove(FIFO_NAMED_MUTEX);
  Trace << std::endl;
}

bool Server::start() {
  try {
    if (!TaskController::create(_options))
      return false;
    _tcpAcceptor = std::make_shared<tcp::TcpAcceptor>(*this);
    if (!_tcpAcceptor->start())
      return false;

    _fifoAcceptor = std::make_shared<fifo::FifoAcceptor>(*this);
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

bool Server::startSession(std::string_view clientId, RunnablePtr session) {
  std::scoped_lock lock(_mutex);
  auto [it, inserted] = _sessions.emplace(clientId, session);
  if (!inserted)
    return false;
  return session->start();
}

void Server::stopSessions() {
  std::scoped_lock lock(_mutex);
  for (auto& pr : _sessions)
    if (auto session = pr.second.lock(); session)
      session->stop();
}
