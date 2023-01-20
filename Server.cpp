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

std::atomic<unsigned> Server::_runningSessions = 0;

Server::Server(const ServerOptions& options) :
  _options(options) {
  StrategySelector strategySelector(options);
  Strategy& strategy = strategySelector.get();
  strategy.set(options);
}

bool Server::start() {
  if (!TaskController::create(_options))
    return false;
  _tcpAcceptor = std::make_shared<tcp::TcpAcceptor>(_options, *this);
  if (!_tcpAcceptor->start())
    return false;

  _fifoAcceptor = std::make_shared<fifo::FifoAcceptor>(_options, *this);
  return _fifoAcceptor->start();
}

void Server::stop() {
  if (_tcpAcceptor)
    _tcpAcceptor->stop();
  if (_fifoAcceptor)
    _fifoAcceptor->stop();
  TaskController::destroy();
}

unsigned Server::registerSession(RunnableWeakPtr weakPtr) {
  std::lock_guard lock(_mutex);
  RunnablePtr session = weakPtr.lock();
  if (!session)
    return 0;
  _totalSessions++;
  std::atomic<STATUS>& status = session->getStatus();
  status = _runningSessions >= _options._maxTotalSessions ?
    STATUS::MAX_TOTAL_SESSIONS : status.load();
  if (status == STATUS::MAX_TOTAL_SESSIONS) {
    auto [it, success] = _waitingSessions.emplace(_mapIndex++, session);
    if (!success) {
      LogError << "_waitingSessions.emplace failed." << std::endl;
      return 0;
    }
  }
  return _totalSessions;
}

void Server::deregisterSession(RunnableWeakPtr weakPtr) {
  std::lock_guard lock(_mutex);
  _totalSessions--;
  auto session = weakPtr.lock();
  if (!session)
    return;
  std::atomic<STATUS>& status = session->getStatus();
  if (status != STATUS::NONE) {
    removeFromMap(session);
    return;
  }
  if (_waitingSessions.empty()) {
    Trace << "_waitingSessions.empty()" << std::endl;
    return;
  }
  auto it = _waitingSessions.begin();
  auto savedWeakPtr = it->second;
  auto savedSession = savedWeakPtr.lock();
  if (savedSession) {
    savedSession->notify();
    _waitingSessions.erase(it);
  }
}

void Server::removeFromMap(RunnablePtr session) {
  for (auto pr : _waitingSessions) {
    auto savedSession = pr.second.lock();
    if (savedSession) {
      if (savedSession == session) {
	_waitingSessions.erase(pr.first);
	return;
      }
    }
  }
}
