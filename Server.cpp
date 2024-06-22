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
#include "Policy.h"
#include "TaskController.h"
#include "TcpAcceptor.h"

Server::Server(Policy& policy) :
  _chronometer(ServerOptions::_timing, __FILE__, __LINE__),
  _threadPoolSession(ServerOptions::_maxTotalSessions) {
  policy.set();
}

Server::~Server() {
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
  _stopped.store(true);
  stopSessions();
  if (_tcpAcceptor)
    _tcpAcceptor->stop();
  if (_fifoAcceptor)
    _fifoAcceptor->stop();
  _threadPoolAcceptor.stop();
  _threadPoolSession.stop();
  TaskController::destroy();
}

bool Server::startSession(RunnablePtr session) {
  std::lock_guard lock(_mutex);
  session->start();
  std::size_t clientId = session->getId();
  auto [it, inserted] = _sessions.emplace(clientId, session);
  if (!inserted)
    return false;
  _threadPoolSession.calculateStatus(session);
  session->sendStatusToClient();
  _threadPoolSession.push(session);
  return true;
}

void Server::stopSessions() {
  std::lock_guard lock(_mutex);
  for (auto& pr : _sessions)
    if (auto session = pr.second.lock(); session)
      session->stop();
}
