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
  std::lock_guard lock(_mutex);
  if (_tcpAcceptor)
    _tcpAcceptor->stop();
  if (_fifoAcceptor)
    _fifoAcceptor->stop();
  TaskController::destroy();
}

void Server::registerSession(RunnablePtr session, ThreadPool& threadPool) {
  std::lock_guard lock(_mutex);
  _totalSessions++;
  if (!session->start())
    return;
  std::atomic<STATUS>& status = session->getStatus();
  if (status != STATUS::NONE) {
    session->_waiting = true;
    auto [it, success] = _waitingSessions.emplace(_mapIndex++, session);
    if (!success) {
      LogError << "_waitingSessions.emplace failed." << std::endl;
    }
  }
  threadPool.push(session);
}

void Server::deregisterSession(RunnableWeakPtr weakPtr) {
  std::lock_guard lock(_mutex);
  struct StopSession {
    StopSession(RunnableWeakPtr weakPtr) : _weakPtr(weakPtr) {}
    ~StopSession() {
      if (auto session = _weakPtr.lock(); session)
	session->stop();
    }
    RunnableWeakPtr _weakPtr;
  } stopSession(weakPtr);
  std::string_view type;
  if (auto session = weakPtr.lock(); session) {
    type = session->getType();
    if (session->_waiting) {
      removeFromMap(session);
      return;
    }
  }
  for (auto it = _waitingSessions.begin(); it != _waitingSessions.end();) {
    if (auto waiting = it->second.lock(); waiting) {
      if (waiting->notify(type)) {
	it = _waitingSessions.erase(it);
	break;
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
    if (auto waiting = it->second.lock(); waiting && waiting == session) {
      _waitingSessions.erase(it);
      break;
    }
  }
}
