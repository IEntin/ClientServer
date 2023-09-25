/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolBase.h"

struct Subtask;
using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;

class Client : private boost::noncopyable {

 protected:
  Client();

  bool processTask(TaskBuilderPtr taskBuilder);
  bool printReply(const HEADER& header, std::string_view buffer);
  void start();

  void displayMaxTotalSessionsWarn();
  void displayMaxSessionsOfTypeWarn(std::string_view type);
  bool displayStatus(STATUS status);

  ThreadPoolBase _threadPoolClient;
  std::atomic<STATUS> _status = STATUS::NONE;
  RunnableWeakPtr _heartbeat;
  TaskBuilderPtr _taskBuilder1;
  TaskBuilderPtr _taskBuilder2;
  static std::atomic<ACTIONS> _signalFlag;
 public:
  virtual ~Client();

  virtual bool send(const Subtask& subtask) = 0;
  virtual bool receive() = 0;
  virtual bool receiveStatus() = 0;
  virtual bool run() = 0;

  void stop();
  static void onSignal();
};
