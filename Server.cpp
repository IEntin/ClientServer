/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Server.h"

#include <filesystem>

#include <boost/interprocess/sync/named_mutex.hpp>

#include "Ad.h"
#include "EchoPolicy.h"
#include "FifoAcceptor.h"
#include "FifoSession.h"
#include "NoSortInputPolicy.h"
#include "ServerOptions.h"
#include "SortInputPolicy.h"
#include "TaskController.h"
#include "TcpAcceptor.h"
#include "TcpSession.h"
#include "Utility.h"

Server::Server() :
  _chronometer(ServerOptions::_timing),
  _threadPoolSession(ServerOptions::_maxTotalSessions) {
  try {
    // create if was removed
    std::string_view fifoDirectoryNameV(Options::_fifoDirectoryName.data(), Options::_fifoDirectoryName.size());
    std::filesystem::create_directory(fifoDirectoryNameV);
    // Unlock computer in case the app crashed during debugging
    // leaving mutex locked, run serverX or testbin in this case.
    removeNamedMutex();
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

Server::~Server() {
  try {
    Ad::clear();
    utility::removeAccess();
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
  }
}

void Server::setPolicy() {
  POLICYENUM policyEnum = ServerOptions::_policyEnum;
  switch (std::to_underlying(policyEnum)) {
  case std::to_underlying(POLICYENUM::NOSORTINPUT) :
    Ad::readAds(ServerOptions::_adsFileName);
    _policy = std::make_unique<NoSortInputPolicy>();
    break;
  case std::to_underlying(POLICYENUM::SORTINPUT) :
    Ad::readAds(ServerOptions::_adsFileName);
    _policy = std::make_unique<SortInputPolicy>();
    break;
  case std::to_underlying(POLICYENUM::ECHOPOLICY) :
    _policy = std::make_unique<EchoPolicy>();
    break;
  default:
    throw std::runtime_error("non-existent policy");
    break;
  }
}

bool Server::start() {
  setPolicy();
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

void Server::createFifoSession(std::string_view pubB,
			       std::string_view signatureWithPubKey) {
  std::lock_guard lock(_mutex);
  auto session =
    std::make_shared<fifo::FifoSession>(weak_from_this(), pubB, signatureWithPubKey);
  startSession(session);
}

void Server::createTcpSession(tcp::ConnectionPtr connection,
			      std::string_view pubB,
			      std::string_view signatureWithPubKey) {
  std::lock_guard lock(_mutex);
  auto session =
    std::make_shared<tcp::TcpSession>(weak_from_this(), connection, pubB, signatureWithPubKey);
  startSession(session);
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
