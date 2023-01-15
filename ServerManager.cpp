/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerManager.h"
#include "FifoAcceptor.h"
#include "ServerOptions.h"
#include "Strategy.h"
#include "TaskController.h"
#include "TcpAcceptor.h"

ServerManager::ServerManager(const ServerOptions& options) :
  _options(options), _strategySelector(options), _strategy(_strategySelector.get()) {
  _strategy.create(options);
}

bool ServerManager::start() {
  if (!TaskController::create(_options))
    return false;
  _tcpAcceptor = std::make_shared<tcp::TcpAcceptor>(_options, *this);
  if (!_tcpAcceptor->start())
    return false;

  _fifoAcceptor = std::make_shared<fifo::FifoAcceptor>(_options, *this);
  return _fifoAcceptor->start();
}

void ServerManager::stop() {
  if (_tcpAcceptor)
    _tcpAcceptor->stop();
  if (_fifoAcceptor)
    _fifoAcceptor->stop();
  TaskController::destroy();
}

std::atomic<unsigned>& ServerManager::totalSessions() {
  return _totalSessions;
}

STATUS ServerManager::incrementTotalSessions() {
  std::lock_guard lock(_mutex);
  _totalSessions++;
  _status = _totalSessions > _options._maxTotalSessions ?
    STATUS::MAX_TOTAL_SESSIONS : STATUS::NONE;
  return _status;
}

STATUS ServerManager::decrementTotalSessions() {
  std::lock_guard lock(_mutex);
  _totalSessions--;
  if (_totalSessions > _options._maxTotalSessions)
    return _status;
  STATUS expected = STATUS::MAX_TOTAL_SESSIONS;
  if (_status.compare_exchange_strong(expected, STATUS::NONE)) {
    _fifoAcceptor->notify();
    _tcpAcceptor->notify();
  }
  return _status;
}
