/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>
#include <memory>

using RunnablePtr = std::shared_ptr<class Runnable>;

class Runnable {
 public:
  explicit Runnable(std::atomic<bool>* stopped = nullptr,
		    std::atomic<int>* totalConnections = nullptr,
		    std::atomic<int>* typedConnections = nullptr);
  virtual ~Runnable();
  virtual void run();
  std::atomic<bool>& _stopped;
  // used for total connections
  std::atomic<int>& _totalConnections;
  // used for specific connection type
  std::atomic<int>& _typedConnections;
 protected:
  std::atomic<bool> _stoppedParent = false;
  std::atomic<int> _totalConnectionsParent;
  std::atomic<int> _typedConnectionsParent;
};
