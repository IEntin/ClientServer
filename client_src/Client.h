/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/core/noncopyable.hpp>

#include "Chronometer.h"
#include "IOUtility.h"
#include "Subtask.h"
#include "ThreadPoolBase.h"
#include "Utility.h"

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
using TaskBuilderWeakPtr = std::weak_ptr<class TaskBuilder>;

class Client;

class Client : private boost::noncopyable {

protected:
  std::string _response;
  std::variant<CryptoPlPlPtr, CryptoSodiumPtr> _crypto;
  Client();

  bool processTask(TaskBuilderWeakPtr weakPtr);
  bool printReply();
  void start();

  void displayMaxTotalSessionsWarn() const;
  void displayMaxSessionsOfTypeWarn(std::string_view type) const;
  bool displayStatus(STATUS status) const;
  bool DHFinish(std::string_view clientIdStr, std::string_view encodedPubAreceived);

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

  std::string_view compressEncrypt(std::string& buffer,
				   const HEADER& header,
				   std::string& data,
				   bool doEncrypt,
				   int compressionLevel = 3);
  virtual bool send(const struct Subtask& subtask) = 0;
  virtual bool receive() = 0;
  virtual bool receiveStatus() = 0;
  virtual void run() = 0;

  void startHeartbeat();
  void stop();
  static void onSignal();
};
