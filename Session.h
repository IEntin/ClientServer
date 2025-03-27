/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <boost/core/noncopyable.hpp>

#include "Crypto.h"
#include "Header.h"
#include "IOUtility.h"

using ServerWeakPtr = std::weak_ptr<class Server>;
using SessionPtr = std::shared_ptr<class Session>;
using TaskPtr = std::shared_ptr<class Task>;

class Session : private boost::noncopyable {
protected:
  std::size_t _clientId = 0;
  CryptoPtr _crypto;
  HEADER _header;
  std::string _request;
  TaskPtr _task;
  std::string _responseData;
  std::string _buffer;
  ServerWeakPtr _server;

  Session(ServerWeakPtr server,
	  std::string_view msgHash,
	  const CryptoPP::SecByteBlock& pubB,
	  std::string_view signatureWithPubKey);
  virtual ~Session() = default;
  std::pair<HEADER, std::string_view>
  buildReply(std::atomic<STATUS>& status);
  bool processTask();

  template <typename L>
  void sendStatusToClient(L& lambda, STATUS status) {
    if (auto server = _server.lock(); server) {
      std::string clientIdStr;
      ioutility::toChars(_clientId, clientIdStr);
      unsigned size = clientIdStr.size();
      const auto& pubA(_crypto->getPubKey());
      HEADER header{ HEADERTYPE::DH_HANDSHAKE, 0, size, CRYPTO::NONE,
		     COMPRESSORS::NONE, DIAGNOSTICS::NONE, status, pubA.size() };
      lambda(header, clientIdStr, pubA);
    }
  }

  void displayCapacityCheck(std::string_view type,
			    unsigned totalNumberObjects,
			    unsigned numberObjects,
			    unsigned numberRunningByType,
			    unsigned maxNumberRunningByType,
			    STATUS status) const;
public:
  virtual void sendStatusToClient() = 0;
  std::size_t getId() const { return _clientId; }
};
