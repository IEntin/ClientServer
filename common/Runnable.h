/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>
#include <memory>

using RunnablePtr = std::shared_ptr<class Runnable>;

class Runnable {
 public:
  explicit Runnable(RunnablePtr stoppedParent = RunnablePtr(),
		    RunnablePtr totalConnectionsParent = RunnablePtr(),
		    RunnablePtr typedConnectionsParent = RunnablePtr());
  virtual ~Runnable();
  virtual void run() = 0;
  std::atomic<bool>& _stopped;
  // used for total connections
  std::atomic<int>& _totalConnections;
  // used for specific connection type
  std::atomic<int>& _typedConnections;
 private:
  std::atomic<bool> _stoppedThis = false;
  std::atomic<int> _totalConnectionsThis = 0;
  std::atomic<int> _typedConnectionsThis = 0;
  const bool _incrementConnections = false;
};
