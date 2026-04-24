/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>

#include <boost/core/noncopyable.hpp>

#include "EncryptorTemplates.h"
#include "IOUtility.h"

using namespace encryptortemplates;

using ServerWeakPtr = std::weak_ptr<class Server>;
using TaskPtr = std::shared_ptr<class Task>;

class Session : private boost::noncopyable {
protected:
  std::size_t _clientId = 0;
  HEADER _header;
  std::string _request;
  TaskPtr _task;
  std::string _responseData;
  std::string _buffer;
  ServerWeakPtr _server;

  CryptoSodiumPtr _primarySodiumEncryptor;
  CryptoSodiumPtr _secondarySodiumEncryptor;
  CryptoPlPlPtr _primaryCryptoppEncryptor;
  CryptoPlPlPtr _secondaryCryptoppEncryptor;

  // key exchange parametera
  std::string _primaryPubKeyAes;
  std::string _secondaryPubKeyAes;

  ENCRYPTORCONTAINER _encryptorContainer;

  Session(ServerWeakPtr server,
	  std::string_view primarySignatureWithKey,
	  std::string_view primaryPubKeyAes,
	  std::string_view secondarySignatureWithKey,
	  std::string_view secondaryPubKeyAes);
  virtual ~Session() = default;
  std::pair<HEADER, std::string_view>
  buildReply(std::atomic<STATUS>& status);
  bool processTask();

  template <typename L>
  void sendStatusToClient(L& lambda, STATUS status) {
    std::string clientIdStr;
    clientIdStr = ioutility::toCharsBoost(_clientId);
    HEADER header;
    std::string encodedPubKeyAes;
    sendStatusToClientImpl(_encryptorContainer,
			   clientIdStr,
			   status,
			   header,
			   encodedPubKeyAes);
    lambda(header, clientIdStr, encodedPubKeyAes);
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
