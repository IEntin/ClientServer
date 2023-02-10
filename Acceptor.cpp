/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Acceptor.h"
#include "Logger.h"
#include "Server.h"
#include <cassert>

Acceptor::Acceptor(const ServerOptions& options, Server& server) :
  _options(options), _server(server),
  _threadPoolAcceptor(_server.getThreadPoolAcceptor()),
  _threadPoolSession(_server.getThreadPoolSession()) {
  ThreadPoolBase::_numberRelatedObjects++;
}

Acceptor::~Acceptor() {
  ThreadPoolBase::_numberRelatedObjects--;
  Trace << std::endl;
}

bool Acceptor::startSession(std::string_view clientId, RunnablePtr session) {
  auto [it, inserted] = _sessions.emplace(clientId, session);
  assert(inserted && "duplicate clientId");
  return session->start();
}

void Acceptor::destroySession(const std::string& clientId) {
  if (auto it = _sessions.find(clientId); it != _sessions.end()) {
    if (auto session = it->second.lock(); session) {
      session->stop();
      _threadPoolSession.removeFromQueue(session);
    }
    _sessions.erase(it);
  }
}

void Acceptor::stop() {
  for (auto& pr : _sessions)
    if (auto session = pr.second.lock(); session)
      session->stop();
  _sessions.clear();
}
