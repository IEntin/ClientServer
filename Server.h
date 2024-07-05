/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <map>

#include "Chronometer.h"
#include "ThreadPoolDiffObj.h"

class Server;
using ServerPtr = std::shared_ptr<Server>;
using ServerWeakPtr = std::weak_ptr<Server>;
using SessionMap = std::map<std::size_t, RunnableWeakPtr>;
using SessionPtr = std::shared_ptr<class Session>;

class Server : public std::enable_shared_from_this<Server> {
public:
  Server(class Policy& policy);
  ~Server();
  bool start();
  void stop();
  bool startSession(RunnablePtr runnable, SessionPtr session);
  void stopSessions();
private:
  Chronometer _chronometer;
  SessionMap _sessions;
  ThreadPoolBase _threadPoolAcceptor;
  ThreadPoolDiffObj _threadPoolSession;
  RunnablePtr _tcpAcceptor;
  RunnablePtr _fifoAcceptor;
  std::mutex _mutex;
  std::condition_variable _startCondition;
  std::atomic<bool> _stopped = false;
};
