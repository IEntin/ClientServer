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
	 << " sessions=" << _typedSessions << '\n';
    if (_max > 0 && _typedSessions > _max) {
      if (_type == TCP) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << "\nnumber running tcp sessions=" << _typedSessions << " at thread pool capacity,\n"
	     << "tcp client will wait in the pool queue.\n"
	     << "close one of running tcp sessions\n"
	     << "or increase \"MaxTcpSessions\" in ServerOptions.json.\n";
	// this will allow the client wait for available thread,
	// client will not close by itself and run when possible.
	return PROBLEMS::MAX_TCP_SESSIONS;
      }
      else if (_type == FIFO) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << "\nnumber fifo sessions=" << _typedSessions 
	     << " at thread pool capacity, client will close.\n"
	     << "increase \"MaxFifoSessions\" in ServerOptions.json.\n";
	// this will throw away session and close the client.
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
