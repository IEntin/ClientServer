/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>

#include <boost/core/noncopyable.hpp>

#include "Header.h"
#include "IOUtility.h"

#ifdef CRYPTOVARIANT
#include "CryptoVariant.h"
#else
#include "CryptoBase.h"
#endif

using ServerWeakPtr = std::weak_ptr<class Server>;
using TaskPtr = std::shared_ptr<class Task>;

class Session : private boost::noncopyable {
protected:
  std::size_t _clientId = 0;
  cryptovariant::CryptoVariant _encryptorContainer;
  HEADER _header;
  std::string _request;
  TaskPtr _task;
  std::string _responseData;
  std::string _buffer;
  ServerWeakPtr _server;
  cryptovariant::CryptoVariant _encryptorVector;

  Session(ServerWeakPtr server,
	  std::string_view encodedPeerPubKeyAes,
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
      HEADER header;
      std::string encodedPubKeyAes;
      cryptovariant::sendStatusToClient(_encryptorContainer,
					clientIdStr,
					status,
					header,
					encodedPubKeyAes);
      lambda(header, clientIdStr, encodedPubKeyAes);
    }
  }

  void displayCapacityCheck(std::string_view type,
			    unsigned totalNumberObjects,
			    unsigned numberObjects,
			    unsigned numberRunningByType,
			    unsigned maxNumberRunningByType,
			    STATUS status);
public:
  std::size_t getId() const { return _clientId; }
};
