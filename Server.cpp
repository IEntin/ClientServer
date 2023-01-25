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
  std::atomic<STATUS>& status = session->getStatus();
  status = _totalSessions > _options._maxTotalSessions ?
    STATUS::MAX_TOTAL_SESSIONS : status.load();
  if (status != STATUS::NONE) {
    session->_waiting = true;
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
  auto caller = weakPtr.lock();
  if (!caller)
    return;
  if (caller->_waiting) {
    removeFromMap(caller);
    caller->_waiting = false;
    caller->_waiting.notify_one();
    return;
  }
  std::string_view stopping = caller->getType();
  for (auto it = _waitingSessions.begin(); it != _waitingSessions.end();) {
    auto savedWeakPtr = it->second;
    auto savedSession = savedWeakPtr.lock();
    if (savedSession) {
      if (savedSession->notify(stopping)) {
	it = _waitingSessions.erase(it);
	return;
      }
      else
	++it;
    }
    else
      it = _waitingSessions.erase(it);
  }
}

void Server::removeFromMap(RunnablePtr session) {
  for (auto it = _waitingSessions.begin(); it != _waitingSessions.end(); ++it) {
    if (auto savedSession = it->second.lock(); savedSession && savedSession == session) {
      _waitingSessions.erase(it);
      break;
    }
  }
}
