/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/core/noncopyable.hpp>

#include "Chronometer.h"
#include "CryptoPlPl.h"
#include "CryptoSodium.h"
#include "IOUtility.h"
#include "Subtask.h"
#include "ThreadPoolBase.h"

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
using TaskBuilderWeakPtr = std::weak_ptr<class TaskBuilder>;

class Client;

class Client : private boost::noncopyable {

protected:
  std::string _response;
  CryptoPlPlPtr _crypto;
  Client();

  bool processTask(TaskBuilderWeakPtr weakPtr);
  bool printReply();
  void start();

  void displayMaxTotalSessionsWarn() const;
  void displayMaxSessionsOfTypeWarn(std::string_view type) const;
  bool displayStatus(STATUS status) const;
  bool DHFinish(std::string_view clientIdStr, std::span<const unsigned char> pubAreceived);

  std::size_t _clientId = 0;
  Chronometer _chronometer;
  ThreadPoolBase _threadPoolClient;
  std::atomic<STATUS> _status = STATUS::NONE;
  HEADER _header;
  RunnableWeakPtr _heartbeat;
  TaskBuilderWeakPtr _taskBuilder;
  Subtasks _task;
  static std::atomic<bool> _closeFlag;
  std::string _buffer;
public:
  virtual ~Client();

  virtual bool send(const struct Subtask& subtask) = 0;
  virtual bool receive() = 0;
  virtual bool receiveStatus() = 0;
  virtual void run() = 0;

  void startHeartbeat();
  void stop();
  static void onSignal();
};
