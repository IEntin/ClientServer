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
		    RunnablePtr totalSessionsParent = RunnablePtr(),
		    RunnablePtr typedSessionsParent = RunnablePtr(),
		    Type type = NONE,
		    int max = 0);
  virtual ~Runnable();
  virtual void run() = 0;
  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual bool killThread() const { return false; }
  PROBLEMS getStatus();
  PROBLEMS checkCapacity();
  std::atomic<bool>& _stopped;
  // used for total sessions
  std::atomic<int>& _totalSessions;
  // used for specific session type
  std::atomic<int>& _typedSessions;
 protected:
  std::atomic<bool> _stoppedThis = false;
  std::atomic<int> _totalSessionsThis = 0;
  std::atomic<int> _typedSessionsThis = 0;
  const bool _countingSessions = false;
  const Type _type;
  std::string _name;
  int _max = 0;
};
