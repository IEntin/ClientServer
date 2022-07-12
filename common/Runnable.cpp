/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"
#include "Header.h"
#include "Utility.h"

Runnable::Runnable(RunnablePtr stoppedParent,
		   RunnablePtr totalConnectionsParent,
		   RunnablePtr typedConnectionsParent,
		   Type type,
		   int max) :
  _stopped(stoppedParent ? stoppedParent->_stopped : _stoppedThis),
  _totalConnections(totalConnectionsParent ? totalConnectionsParent->_totalConnections : _totalConnectionsThis),
  _typedConnections(typedConnectionsParent ? typedConnectionsParent->_typedConnections : _typedConnectionsThis),
  // only connections (sessions), children of children, have both parents.
  _countingConnections(totalConnectionsParent && typedConnectionsParent),
  _type(type),
  _max(max) {
  if (_type == FIFO)
    _name = "fifo";
  else if (_type == TCP)
    _name = "tcp";
}

Runnable::~Runnable() {
  if (_countingConnections) {
    _totalConnections--;
    _typedConnections--;
  }
}

PROBLEMS Runnable::checkCapacity() {
  if (_countingConnections) {
    _totalConnections++;
    _typedConnections++;
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ":\ntotal connections=" << _totalConnections << ',' << _name
	 << " connections=" << _typedConnections << '\n';
    if (_max > 0 && _typedConnections > _max) {
      if (_type == TCP) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << "\nnumber running tcp connections=" << _typedConnections << " at thread pool capacity,\n"
	     << "tcp client will wait in the pool queue.\n"
	     << "close one of running tcp connections\n"
	     << "or increase \"MaxTcpConnections\" in ServerOptions.json.\n";
	// this will allow the client wait for available thread,
	// client will not close by itself and run when possible.
	return PROBLEMS::MAX_TCP_CONNECTIONS;
      }
      else if (_type == FIFO) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << "\nnumber fifo connections=" << _typedConnections 
	     << " at thread pool capacity, client will close.\n"
	     << "increase \"MaxFifoConnections\" in ServerOptions.json.\n";
	// this will throw away connection and close the client.
	return PROBLEMS::MAX_FIFO_CONNECTIONS;
      }
    }
  }
  return PROBLEMS::NONE;
}
