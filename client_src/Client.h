/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Chronometer.h"
#include "CryptoNS.h"
#include "ThreadPoolBase.h"

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
using TaskBuilderWeakPtr = std::weak_ptr<class TaskBuilder>;

class Client;

class Client {

protected:
  std::string _response;
  CryptoPtr _crypto;
  Client();

  bool processTask(TaskBuilderWeakPtr weakPtr);
  bool printReply();
  void start();

  void displayMaxTotalSessionsWarn() const;
  void displayMaxSessionsOfTypeWarn(std::string_view type) const;
  bool displayStatus(STATUS status) const;
  std::size_t _clientId;
  Chronometer _chronometer;
  ThreadPoolBase _threadPoolClient;
  std::atomic<STATUS> _status = STATUS::NONE;
  HEADER _header;
  RunnableWeakPtr _heartbeat;
  TaskBuilderWeakPtr _taskBuilder;
  static std::atomic<bool> _closeFlag;
public:
  virtual ~Client();

  virtual bool send(struct Subtask& subtask) = 0;
  virtual bool receive() = 0;
  virtual bool receiveStatus() = 0;
  virtual void run() = 0;

  void stop();
  static void onSignal();
};
