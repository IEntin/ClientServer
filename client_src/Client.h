/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "ThreadPool.h"
#include <atomic>

struct ClientOptions;

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;

class Client {

 protected:

  Client(const ClientOptions& options);

  bool processTask(TaskBuilderPtr taskBuilder);

  bool printReply(const std::vector<char>& buffer, const HEADER& header);

  void startHeartbeat();

  const ClientOptions& _options;

  ThreadPool _threadPoolTaskBuilder;

  std::atomic<STATUS> _status = STATUS::NONE;

  std::atomic_flag _stopFlagWait = ATOMIC_FLAG_INIT;

  static std::atomic_flag _stopFlag;

  static std::string _clientId;

  static std:: string _serverHost;

  static std::string _tcpPort;

 public:

  virtual ~Client();

  virtual bool send(const std::vector<char>& msg) = 0;

  virtual bool receive() = 0;

  bool run();

  static bool closeSession();

  static void setStopFlag();

  static bool stopped();

};
