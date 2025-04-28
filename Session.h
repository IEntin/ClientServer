/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>

#include <boost/core/noncopyable.hpp>

#include "CryptoPlPl.h"
#include "Header.h"
#include "IOUtility.h"

using CryptoPtr = std::shared_ptr<CryptoPlPl>;
using ServerWeakPtr = std::weak_ptr<class Server>;
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
	  const std::vector<unsigned char> pubBvector,
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
      std::vector<unsigned char> pubAvector;
      _crypto->getPubKey(pubAvector);
      HEADER header{ HEADERTYPE::DH_HANDSHAKE, 0, size, CRYPTO::NONE,
		     COMPRESSORS::NONE, DIAGNOSTICS::NONE, status, pubAvector.size() };
      lambda(header, clientIdStr, pubAvector);
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
