/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Server.h"
#include "Crypto.h"
#include "FifoAcceptor.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "Strategy.h"
#include "TaskController.h"
#include "TcpAcceptor.h"
#include <boost/interprocess/sync/named_mutex.hpp>

Server::Server(StrategyPtr strategy) :
  _chronometer(ServerOptions::_timing, __FILE__, __LINE__),
  _threadPoolSession(ServerOptions::_maxTotalSessions, &Runnable::sendStatusToClient),
  _strategy(std::move(strategy)) {
  boost::interprocess::named_mutex::remove(FIFO_NAMED_MUTEX);
}

Server::~Server() {
  boost::interprocess::named_mutex::remove(FIFO_NAMED_MUTEX);
  Trace << '\n';
}

bool Server::start() {
  _strategy->set();
  try {
    CryptoKey::initialize(std::any_cast<const ServerOptions&>(ServerOptions::_self));
    if (ServerOptions::_showKey)
      CryptoKey::showKey();
    if (!TaskController::create())
      return false;
    _tcpAcceptor = std::make_shared<tcp::TcpAcceptor>(*this);
    if (!_tcpAcceptor->start())
      return false;
    _threadPoolAcceptor.push(_tcpAcceptor);
    _fifoAcceptor = std::make_shared<fifo::FifoAcceptor>(*this);
    if (!_fifoAcceptor->start())
      return false;
    _threadPoolAcceptor.push(_fifoAcceptor);
    return true;
  }
  catch (const std::bad_any_cast& e) {
    LogError << e.what() << '\n';
    return false;
  }
}

void Server::stop() {
  if (_tcpAcceptor)
    _tcpAcceptor->stop();
  if (_fifoAcceptor)
    _fifoAcceptor->stop();
  _threadPoolAcceptor.stop();
  _threadPoolSession.stop();
  TaskController::destroy();
  std::filesystem::remove(CRYPTO_KEY_FILE_NAME);
}

bool Server::startSession(RunnablePtr session) {
  session->start();
  std::string_view clientId = session->getId();
  std::lock_guard lock(_mutex);
  auto [it, inserted] = _sessions.emplace(clientId, session);
  if (!inserted)
    return false;
  _threadPoolSession.push(session);
  return true;
}

void Server::stopSessions() {
  std::lock_guard lock(_mutex);
  for (auto& pr : _sessions)
    if (auto session = pr.second.lock(); session)
      session->stop();
}
