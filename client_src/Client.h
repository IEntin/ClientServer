/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/core/noncopyable.hpp>

#include "Chronometer.h"
#include "CryptoPlPl.h"
#include "CryptoSodium.h"
#include "EncryptorTemplates.h"
#include "IOUtility.h"
#include "Subtask.h"
#include "ThreadPoolBase.h"

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
using TaskBuilderWeakPtr = std::weak_ptr<class TaskBuilder>;

class Client : private boost::noncopyable {

friend class TaskBuilder;

protected:

  std::string _response;
  encryptortemplates::ENCRYPTORCONTAINER _encryptorContainer;

  CryptoSodiumPtr _primarySodiumEncryptor;
  CryptoSodiumPtr _secondarySodiumEncryptor;
  CryptoPlPlPtr _primaryCryptoppEncryptor;
  CryptoPlPlPtr _secondaryCryptoppEncryptor;

  //authentication parametera
  std::string _primarySignatureWithKey;
  std::string _primaryPubKeyAes;
  std::string _secondarySignatureWithKey;
  std::string _secondaryPubKeyAes;
  HEADER _authenticationHeader;

  Client();
  virtual ~Client();

  void start();

  void stop();

  bool processTask(TaskBuilderWeakPtr weakPtr);
  bool printReply();

  bool sendSignatureCommon();
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
  static thread_local std::string _buffer;

  bool processStatus(std::string_view encodedPeerPubKeyAes,
		     std::string_view type);
  
void clientKeyExchange(std::string_view encodedPeerPubKeyAes);

public:
  virtual bool send(const struct Subtask& subtask) = 0;
  virtual bool receive() = 0;
  virtual bool receiveStatus() = 0;
  virtual void run() = 0;

  void startHeartbeat();
  static void onSignal();
};
