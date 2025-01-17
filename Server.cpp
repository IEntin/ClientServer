/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Server.h"

#include <boost/interprocess/sync/named_mutex.hpp>

#include "Ad.h"
#include "FifoAcceptor.h"
#include "FifoSession.h"
#include "Logger.h"
#include "Policy.h"
#include "ServerOptions.h"
#include "TaskController.h"
#include "TcpAcceptor.h"
#include "TcpSession.h"

Server::Server(Policy& policy) :
  _chronometer(ServerOptions::_timing),
  _threadPoolSession(ServerOptions::_maxTotalSessions) {
  // Unlock computer in case the app crashed during debugging
  // leaving mutex locked, run serverX or testbin in this case
  // so that all processes are on the same host rather than
  // rebooting machine:
  removeNamedMutex();
  policy.set();
}

Server::~Server() {
  Ad::clear();
  utility::removeAccess();
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
  stopSessions();
  if (_tcpAcceptor)
    _tcpAcceptor->stop();
  if (_fifoAcceptor)
    _fifoAcceptor->stop();
  _threadPoolAcceptor.stop();
  _threadPoolSession.stop();
  TaskController::destroy();
}
void Server::createFifoSession(std::string_view msgHash,
			       const CryptoPP::SecByteBlock& pubB,
			       std::string_view rsaPubBserialized) {
  std::lock_guard lock(_mutex);
  auto session = std::make_shared<fifo::FifoSession>(weak_from_this(), msgHash, pubB, rsaPubBserialized);
  startSession(session, session);
}

void Server::createTcpSession(tcp::ConnectionPtr connection,
			      std::string_view msgHash,
			      const CryptoPP::SecByteBlock& pubB,
			      std::string_view rsaPubB) {
  std::lock_guard lock(_mutex);
  auto session =
    std::make_shared<tcp::TcpSession>(weak_from_this(), connection, msgHash, pubB, rsaPubB);
  startSession(session, session);
}

bool Server::startSession(RunnablePtr runnable, SessionPtr session) {
  runnable->start();
  std::size_t clientId = session->getId();
  auto [it, inserted] = _sessions.emplace(clientId, runnable);
  if (!inserted)
    return false;
  _threadPoolSession.calculateStatus(runnable);
  session->sendStatusToClient();
  _threadPoolSession.push(runnable);
  return true;
}

void Server::stopSessions() {
  std::lock_guard lock(_mutex);
  for (auto& pr : _sessions)
    if (auto session = pr.second.lock(); session)
      session->_stopped.store(true);
  for (auto& pr : _sessions)
    if (auto session = pr.second.lock(); session)
      session->stop();
}

void Server::removeNamedMutex() {
  boost::interprocess::named_mutex::remove(FIFO_NAMED_MUTEX);
}
