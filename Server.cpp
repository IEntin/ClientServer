/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Server.h"

#include <filesystem>
#include <boost/interprocess/sync/named_mutex.hpp>

#include "Ad.h"
#include "FifoAcceptor.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "Strategy.h"
#include "TaskController.h"
#include "TcpAcceptor.h"

Server::Server(Strategy& strategy) :
  _chronometer(ServerOptions::_timing, __FILE__, __LINE__),
  _threadPoolSession(ServerOptions::_maxTotalSessions) {
  boost::interprocess::named_mutex::remove(FIFO_NAMED_MUTEX);
  strategy.set();
}

Server::~Server() {
  boost::interprocess::named_mutex::remove(FIFO_NAMED_MUTEX);
  Ad::clear();
  Trace << '\n';
}

bool Server::start() {
  if (!TaskController::create())
    return false;
  _tcpAcceptor = std::make_shared<tcp::TcpAcceptor>(shared_from_this());
  if (!_tcpAcceptor->start())
    return false;
  _threadPoolAcceptor.push(_tcpAcceptor);
  _fifoAcceptor = std::make_shared<fifo::FifoAcceptor>(shared_from_this());
  if (!_fifoAcceptor->start())
    return false;
  _threadPoolAcceptor.push(_fifoAcceptor);
  return true;
}

void Server::stop() {
  setStarted();
  _stopped.store(true);
  if (_tcpAcceptor)
    _tcpAcceptor->stop();
  if (_fifoAcceptor)
    _fifoAcceptor->stop();
  _threadPoolAcceptor.stop();
  _threadPoolSession.stop();
  TaskController::destroy();
}

bool Server::startSession(RunnableWeakPtr sessionWeak) {
  std::lock_guard lock(_mutex);
  if (auto session = sessionWeak.lock(); session) {
    session->start();
    std::size_t clientId = session->getId();
    auto [it, inserted] = _sessions.emplace(clientId, session);
    if (!inserted)
      return false;
    _threadPoolSession.calculateStatus(session);
    session->sendStatusToClient();
    _threadPoolSession.push(session);
  }
  return true;
}

bool Server::removeFromSessions(std::size_t clientId) {
  std::lock_guard lock(_mutex);
  return _sessions.erase(clientId) > 0;
}

void Server::stopSessions() {
  std::lock_guard lock(_mutex);
  for (auto& pr : _sessions)
    if (auto session = pr.second.lock(); session)
      session->stop();
}

void Server::lockStartMutex() {
  std::unique_lock lock(_startMutex);
  _startCondition.wait(lock,  [this] { return _started || _stopped; });
}

void Server::setStarted() {
  std::lock_guard lock(_startMutex);
  _started.store(true);
  _startCondition.notify_all();
}
