/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/core/noncopyable.hpp>

#include "Chronometer.h"
#include "EncryptorTemplates.h"
#include "IOUtility.h"
#include "Subtask.h"
#include "ThreadPoolBase.h"

#include "CryptoVariant.h"

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
using TaskBuilderWeakPtr = std::weak_ptr<class TaskBuilder>;

class Client : private boost::noncopyable {

protected:

  std::string _response;
  ENCRYPTORCONTAINER _encryptorContainer;

Client();

  bool processTask(TaskBuilderWeakPtr weakPtr);
  bool printReply();
  void start();

  void displayMaxTotalSessionsWarn() const;
  void displayMaxSessionsOfTypeWarn(std::string_view type) const;
  bool displayStatus(STATUS status) const;
  
  std::size_t _clientId = 0;
  Chronometer _chronometer;
  ThreadPoolBase _threadPoolClient;
  std::atomic<STATUS> _status = STATUS::NONE;
  HEADER _header;
  RunnableWeakPtr _heartbeat;
  TaskBuilderWeakPtr _taskBuilder;
  static thread_local Subtasks _task;
  static std::atomic<bool> _closeFlag;
  std::string _buffer;

  bool processStatus(std::string_view encodedPeerPubKeyAes,
		     std::string_view type) {
 _status = extractStatus(_header);
  try {
    cryptovariant::clientKeyExchangeContainer(_encryptorContainer, encodedPeerPubKeyAes);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return false;
  }
  switch (_status) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    displayMaxSessionsOfTypeWarn(type);
  break;
  case STATUS::MAX_TOTAL_OBJECTS:
    displayMaxTotalSessionsWarn();
    break;
  default:
    break;
  }
  startHeartbeat();
  return true;
}

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
