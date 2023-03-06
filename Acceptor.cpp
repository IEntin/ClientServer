/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Acceptor.h"
#include "ThreadPoolBase.h"
#include "ThreadPoolDiffObj.h"
#include "Logger.h"
#include <cassert>

Acceptor::Acceptor(const ServerOptions& options,
		   ThreadPoolBase& threadPoolAcceptor,
		   ThreadPoolDiffObj& threadPoolSession) :
  _options(options),
  _threadPoolAcceptor(threadPoolAcceptor),
  _threadPoolSession(threadPoolSession) {}

Acceptor::~Acceptor() {
  Trace << std::endl;
}

bool Acceptor::startSession(std::string_view clientId, RunnablePtr session) {
  auto [it, inserted] = _sessions.emplace(clientId, session);
  if (!inserted)
    return false;
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
