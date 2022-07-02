/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Runnable.h"

Runnable::Runnable(std::atomic<bool>* stopped, std::atomic<int>* totalConnections, std::atomic<int>* typedConnections) :
  _stopped(stopped ? *stopped : _stoppedParent),
  _totalConnections(totalConnections ? *totalConnections : _totalConnectionsParent),
  _typedConnections(typedConnections ? *typedConnections : _typedConnectionsParent) {
  _totalConnections++;
  _typedConnections++;
}

Runnable::~Runnable() {
  _totalConnections--;
  _typedConnections--;
}

void Runnable::run() {}
