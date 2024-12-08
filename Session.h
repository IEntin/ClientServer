/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Crypto.h"
#include "Header.h"
#include "IOUtility.h"

class Server;

using Response = std::vector<std::string>;
using ServerWeakPtr = std::weak_ptr<Server>;
using SessionPtr = std::shared_ptr<class Session>;
using TaskPtr = std::shared_ptr<class Task>;

class Session {
protected:
  std::size_t _clientId;
  CryptoPtr _crypto;
  std::string _request;
  Response _response;
  TaskPtr _task;
  std::string _responseData;
  ServerWeakPtr _server;
  static thread_local std::string _buffer;

  Session(ServerWeakPtr server,
	  const CryptoPP::SecByteBlock& pubB,
	  std::string_view rsaPubBserialized);
  virtual ~Session();
  std::string_view buildReply(std::atomic<STATUS>& status);
  bool processTask();

  template <typename L>
  void sendStatusToClient(L& lambda, STATUS status) {
    if (auto server = _server.lock(); server) {
      std::string clientIdStr;
      ioutility::toChars(_clientId, clientIdStr);
      unsigned size = clientIdStr.size();
      const auto& pubA(_crypto->getPubKey());
      HEADER header
	{ HEADERTYPE::DH_HANDSHAKE, size, COMPRESSORS::NONE, DIAGNOSTICS::NONE, status, pubA.size() };
      lambda(header, clientIdStr, pubA);
    }
  }

  void displayCapacityCheck(unsigned totalNumberObjects,
			    unsigned numberObjects,
			    unsigned numberRunningByType,
			    unsigned maxNumberRunningByType,
			    STATUS status) const;
public:
  virtual void sendStatusToClient() = 0;
  virtual std::string_view getDisplayName() const = 0;
  std::size_t getId() const { return _clientId; }
};
