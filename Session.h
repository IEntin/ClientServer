/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>

#include <boost/core/noncopyable.hpp>

#include "CryptoDefinitions.h"
#include "Header.h"
#include "IOUtility.h"

using ServerWeakPtr = std::weak_ptr<class Server>;
using TaskPtr = std::shared_ptr<class Task>;

class Session : private boost::noncopyable {
protected:
  std::size_t _clientId = 0;
  std::variant<CryptoPlPlPtr, CryptoSodiumPtr> _crypto;
  HEADER _header;
  std::string _request;
  TaskPtr _task;
  std::string _responseData;
  std::string _buffer;
  ServerWeakPtr _server;

  Session(ServerWeakPtr server,
	  std::string_view msgHash,
	  std::string_view encodedPubKeyAesClient,
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
      std::string encodedPubKeyAesServer;
      cryptodefinitions::sendStatusToClient(_crypto,
					    clientIdStr,
					    status,
					    header,
					    encodedPubKeyAesServer);
      lambda(header, clientIdStr, encodedPubKeyAesServer);
    }
  }

  void displayCapacityCheck(std::string_view type,
			    unsigned totalNumberObjects,
			    unsigned numberObjects,
			    unsigned numberRunningByType,
			    unsigned maxNumberRunningByType,
			    STATUS status) const;
public:
  std::size_t getId() const { return _clientId; }
};
