/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"
#include "Utility.h"

Runnable::Runnable(RunnablePtr stoppedParent,
		   RunnablePtr totalConnectionsParent,
		   RunnablePtr typedConnectionsParent,
		   Type type,
		   int max) :
  _stopped(stoppedParent ? stoppedParent->_stopped : _stoppedThis),
  _totalConnections(totalConnectionsParent ? totalConnectionsParent->_totalConnections : _totalConnectionsThis),
  _typedConnections(typedConnectionsParent ? typedConnectionsParent->_typedConnections : _typedConnectionsThis),
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

bool Runnable::checkCapacity() {
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
	return false;
      }
      else if (_type == FIFO) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	     << "\nnumber running fifo connections=" << _typedConnections 
	     << " at thread pool capacity, client will close.\n"
	     << "increase \"MaxTcpConnections\" in ServerOptions.json.\n";
	return true;
      }
    }
  }
  return false;
}
