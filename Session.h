/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "cryptopp/eccrypto.h"
#include <cryptopp/integer.h>

#include "Header.h"

class Server;

using Response = std::vector<std::string>;
using ServerWeakPtr = std::weak_ptr<Server>;
using TaskPtr = std::shared_ptr<class Task>;

class Session {
 protected:
  std::size_t _clientId;

  CryptoPP::ECDH<CryptoPP::ECP>::Domain _dhA;
  CryptoPP::SecByteBlock _privA;
  CryptoPP::SecByteBlock _pubA;
  const bool _generatedKeyPair;
  const std::string _pubAstrng;
  CryptoPP::SecByteBlock _sharedA;

  std::string _request;
  Response _response;
  TaskPtr _task;
  std::string _responseData;
  ServerWeakPtr _server;

  Session(ServerWeakPtr server, std::string_view pubBstring);
  virtual ~Session();
  std::string_view buildReply(HEADER& header, std::atomic<STATUS>& status);
  bool processTask(const HEADER& header);
};
