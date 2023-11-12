/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Chronometer.h"
#include "ThreadPoolBase.h"

struct Subtask;
using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
using TaskBuilderWeakPtr = std::weak_ptr<class TaskBuilder>;

class Client {

 protected:
  Client();

  bool processTask(TaskBuilderWeakPtr weakPtr);
  bool printReply(const HEADER& header, std::string_view buffer);
  void start();

  void displayMaxTotalSessionsWarn();
  void displayMaxSessionsOfTypeWarn(std::string_view type);
  bool displayStatus(STATUS status);
  Chronometer _chronometer;
  ThreadPoolBase _threadPoolClient;
  std::atomic<STATUS> _status = STATUS::NONE;
  RunnableWeakPtr _heartbeat;
  TaskBuilderWeakPtr _taskBuilder1;
  TaskBuilderWeakPtr _taskBuilder2;
  std::atomic<bool> _signalFlag = false;
 public:
  virtual ~Client();

  virtual bool send(const Subtask& subtask) = 0;
  virtual bool receive() = 0;
  virtual bool receiveStatus() = 0;
  virtual bool run() = 0;

  void stop();
  static void onSignal(std::atomic<Client*>& clientPtr);
  virtual void close() = 0;
};
