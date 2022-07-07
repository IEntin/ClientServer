/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"
#include "Utility.h"

Runnable::Runnable(RunnablePtr stoppedParent,
		   RunnablePtr totalConnectionsParent,
		   RunnablePtr typedConnectionsParent,
		   const std::string& name,
		   int max) :
  _stopped(stoppedParent ? stoppedParent->_stopped : _stoppedThis),
  _totalConnections(totalConnectionsParent ? totalConnectionsParent->_totalConnections : _totalConnectionsThis),
  _typedConnections(typedConnectionsParent ? typedConnectionsParent->_typedConnections : _typedConnectionsThis),
  _countingConnections(totalConnectionsParent && typedConnectionsParent),
  _name(name),
  _max(max) {}

Runnable::~Runnable() {
  if (_countingConnections && _running) {
    _totalConnections--;
    _typedConnections--;
  }
}

void Runnable::setRunning() {
  if (_running)
    return;
  _running.store(true);
  if (_countingConnections) {
    _totalConnections++;
    _typedConnections++;
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ":total connections=" << _totalConnections << ',' << _name
	 << " connections=" << _typedConnections << '\n';
  }
}
