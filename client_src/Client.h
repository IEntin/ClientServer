/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "cryptopp/eccrypto.h"

#include "Chronometer.h"
#include "ThreadPoolBase.h"

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
using TaskBuilderWeakPtr = std::weak_ptr<class TaskBuilder>;

constexpr const char* FIFO_NAMED_MUTEX{ "FIFO_NAMED_MUTEX" };

class Client;

class Client {

protected:
  CryptoPP::ECDH<CryptoPP::ECP>::Domain _dhB;
  CryptoPP::SecByteBlock _privB;
  CryptoPP::SecByteBlock _pubB;
  const bool _generatedKeyPair;
  CryptoPP::SecByteBlock _sharedB;
  std::string _response;
  Client();

  bool processTask(TaskBuilderWeakPtr weakPtr);
  bool printReply(const HEADER& header, std::string_view buffer) const;
  void start();

  void displayMaxTotalSessionsWarn() const;
  void displayMaxSessionsOfTypeWarn(std::string_view type) const;
  bool displayStatus(STATUS status) const;
  std::size_t _clientId;
  Chronometer _chronometer;
  ThreadPoolBase _threadPoolClient;
  std::atomic<STATUS> _status = STATUS::NONE;
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
