/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>
#include <memory>
#include <string>

enum class PROBLEMS : char;

class Runnable;

using RunnablePtr = std::shared_ptr<Runnable>;

using RunnableWeakPtr = std::weak_ptr<Runnable>;

class Runnable {
 public:
  enum Type { NONE, FIFO, TCP };
  explicit Runnable(RunnablePtr stoppedParent = RunnablePtr(),
		    RunnablePtr totalConnectionsParent = RunnablePtr(),
		    RunnablePtr typedConnectionsParent = RunnablePtr(),
		    Type type = NONE,
		    int max = 0);
  virtual ~Runnable();
  virtual void run() = 0;
  virtual bool start() = 0;
  virtual void stop() = 0;
  PROBLEMS checkCapacity();
  std::atomic<bool>& _stopped;
  // used for total connections
  std::atomic<int>& _totalConnections;
  // used for specific connection type
  std::atomic<int>& _typedConnections;
 protected:
  std::atomic<bool> _stoppedThis = false;
  std::atomic<int> _totalConnectionsThis = 0;
  std::atomic<int> _typedConnectionsThis = 0;
  const bool _countingConnections = false;
  const Type _type;
  std::string _name;
  int _max = 0;
};
