/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"
#include "Header.h"
#include "Utility.h"

Runnable::Runnable(RunnablePtr stoppedParent,
		   RunnablePtr totalSessionsParent,
		   RunnablePtr typedSessionsParent,
		   Type type,
		   int max) :
  _stopped(stoppedParent ? stoppedParent->_stopped : _stoppedThis),
  _totalSessions(totalSessionsParent ? totalSessionsParent->_totalSessions : _totalSessionsThis),
  _typedSessions(typedSessionsParent ? typedSessionsParent->_typedSessions : _typedSessionsThis),
  // only sessions, children of children, have both parents.
  _countingSessions(totalSessionsParent && typedSessionsParent),
  _type(type),
  _max(max) {
  if (_type == FIFO)
    _name = "fifo";
  else if (_type == TCP)
    _name = "tcp";
}

Runnable::~Runnable() {
  if (_countingSessions) {
    _totalSessions--;
    _typedSessions--;
  }
}

PROBLEMS Runnable::checkCapacity() {
  if (_countingSessions) {
    _totalSessions++;
    _typedSessions++;
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ":\ntotal sessions=" << _totalSessions << ',' << _name
	 << " sessions=" << _typedSessions << std::endl;
    if (_max > 0 && _typedSessions > _max) {
      if (_type == TCP) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << "\nNumber of running tcp sessions=" << _typedSessions
	     << " at thread pool capacity.\n"
	     << "The client will wait in the queue.\n"
	     << "Close one of running tcp clients\n"
	     << "or increase \"MaxTcpSessions\" in ServerOptions.json.\n"
	     << "You can also close this client and try again later.\n";
	return PROBLEMS::MAX_TCP_SESSIONS;
      }
      else if (_type == FIFO) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << "\nNumber of running fifo sessions=" << _typedSessions
	     << " at thread pool capacity.\n"
	     << "The client will wait in the queue.\n"
	     << "Close one of running fifo clients\n"
	     << "or increase \"MaxFifoSessions\" in ServerOptions.json.\n"
	     << "You can also close this client and try again later.\n";
	return PROBLEMS::MAX_FIFO_SESSIONS;
      }
    }
  }
  return PROBLEMS::NONE;
}

PROBLEMS Runnable::getStatus() {
  return _typedSessions >= _max ?
    (_type == FIFO ? PROBLEMS::MAX_FIFO_SESSIONS : PROBLEMS::MAX_TCP_SESSIONS) : PROBLEMS::NONE;
}
