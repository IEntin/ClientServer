/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "ThreadPool.h"
#include <atomic>

struct ClientOptions;

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
 
namespace tcp {
  class TcpClientHeartbeat;
}

using TcpClientHeartbeatPtr = std::shared_ptr<class tcp::TcpClientHeartbeat>;

class Client {

 protected:

  Client(const ClientOptions& options);

  bool processTask(TaskBuilderPtr taskBuilder);

  bool printReply(const std::vector<char>& buffer, const HEADER& header);

  void start();

  const ClientOptions& _options;

  ThreadPool _threadPoolTaskBuilder;

  std::atomic<STATUS> _status = STATUS::NONE;

  std::atomic_flag _stopFlagWait = ATOMIC_FLAG_INIT;

  static std::atomic_flag _stopFlag;

  std::string _clientId;

  TcpClientHeartbeatPtr _heartbeat;

 public:

  virtual ~Client();

  virtual bool send(const std::vector<char>& msg) = 0;

  virtual bool receive() = 0;

  virtual bool receiveStatus() = 0;

  virtual bool run() = 0;

  virtual bool destroySession() = 0;

  static void setStopFlag();

};
