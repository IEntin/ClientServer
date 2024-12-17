/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Chronometer.h"
#include "Crypto.h"
#include "IOUtility.h"
#include "Subtask.h"
#include "ThreadPoolBase.h"

using TaskBuilderPtr = std::shared_ptr<class TaskBuilder>;
using TaskBuilderWeakPtr = std::weak_ptr<class TaskBuilder>;

class Client;

class Client {

protected:
  std::string _response;
  CryptoPtr _crypto;
  CryptoWeakPtr _cryptoWeak;
  Client();

  bool processTask(TaskBuilderWeakPtr weakPtr);
  bool printReply();
  void start();

  void displayMaxTotalSessionsWarn() const;
  void displayMaxSessionsOfTypeWarn(std::string_view type) const;
  bool displayStatus(STATUS status) const;

  template <typename L>
  bool init(L& lambda) {
    if (auto crypto = _cryptoWeak.lock(); crypto) {
      const auto& pubKey = _crypto->getPubKey();
      std::size_t pubKeySz = pubKey.size();
      _crypto->signMessage();
      std::string_view signatureWithPubKey = _crypto->getSignatureWithPubKey();
      std::size_t signatureDataSz = signatureWithPubKey.size();
      HEADER header =
	{ HEADERTYPE::DH_INIT, pubKeySz, COMPRESSORS::NONE, DIAGNOSTICS::NONE, _status, signatureDataSz };
      return lambda(header, pubKey, signatureWithPubKey);
    }
    return false;
  }

  bool DHFinish(std::string_view clientIdStr, const CryptoPP::SecByteBlock& pubAreceived);

  std::size_t _clientId;
  Chronometer _chronometer;
  ThreadPoolBase _threadPoolClient;
  std::atomic<STATUS> _status = STATUS::NONE;
  HEADER _header;
  RunnableWeakPtr _heartbeat;
  TaskBuilderWeakPtr _taskBuilder;
  Subtasks _task;
  static std::atomic<bool> _closeFlag;
  static thread_local std::string _buffer;
public:
  virtual ~Client();

  virtual bool send(struct Subtask& subtask) = 0;
  virtual bool receive() = 0;
  virtual bool receiveStatus() = 0;
  virtual void run() = 0;

  static std::string& getBuffer() { return _buffer; }
  void startHeartbeat();
  void stop();
  static void onSignal();
};
