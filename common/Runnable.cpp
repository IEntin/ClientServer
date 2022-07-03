/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"

Runnable::Runnable(RunnablePtr stoppedParent,
		   RunnablePtr totalConnectionsParent,
		   RunnablePtr typedConnectionsParent) :
  _stopped(stoppedParent ? stoppedParent->_stopped : _stoppedThis),
  _totalConnections(totalConnectionsParent ? totalConnectionsParent->_totalConnections : _totalConnectionsThis),
  _typedConnections(typedConnectionsParent ? typedConnectionsParent->_typedConnections : _typedConnectionsThis),
  _stoppedOwner(!stoppedParent),
  _totalConnectionsOwner(!totalConnectionsParent),
  _typedConnectionsOwner(!typedConnectionsParent) {
  if (!_totalConnectionsOwner)
    _totalConnections++;
  if (!_typedConnectionsOwner)
    _typedConnections++;
}

Runnable::~Runnable() {
  if (!_totalConnectionsOwner)
    _totalConnections--;
  if (!_typedConnectionsOwner)
    _typedConnections--;
}

// implemented for fake runnables
void Runnable::run() {}
