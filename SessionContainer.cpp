/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "SessionContainer.h"
#include "ServerOptions.h"

SessionContainer::SessionContainer(const ServerOptions& options) :
  _options(options) {
}

std::atomic<unsigned>& SessionContainer::totalSessions() {
  return _totalSessions;
}

STATUS SessionContainer::incrementTotalSessions() {
  std::lock_guard lock(_mutex);
  _totalSessions++;
  _status = _totalSessions > _options._maxTotalSessions ?
    STATUS::MAX_TOTAL_SESSIONS : STATUS::NONE;
  return _status;
}

STATUS SessionContainer::decrementTotalSessions() {
  std::lock_guard lock(_mutex);
  _totalSessions--;
  if (_totalSessions > _options._maxTotalSessions)
    return _status;
  STATUS expected = STATUS::MAX_TOTAL_SESSIONS;
  if (_status.compare_exchange_strong(expected, STATUS::NONE)) {
    for (auto& [ key, weakPtr ] : _fifoSessions) {
      auto session = weakPtr.lock();
      if (session)
	session->notify();
    }
    for (auto& [ key, weakPtr ] : _tcpSessions) {
      auto session = weakPtr.lock();
      if (session)
	session->notify();
    }
  }
  return _status;
}
