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

class Server : public std::enable_shared_from_this<Server> {
public:
  Server(class Strategy& strategy);
  ~Server();
  bool start();
  void stop();
  bool startSession(RunnableWeakPtr sessionWeak);
  bool removeFromSessions(std::size_t clientId);
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
  std::atomic<bool> _started = false;
  std::atomic<bool> _stopped = false;
};
