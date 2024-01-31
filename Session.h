/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <cryptopp/integer.h>

#include "Header.h"

class Server;

using Response = std::vector<std::string>;
using TaskPtr = std::shared_ptr<class Task>;

class Session {
 protected:
  std::size_t _clientId;
  CryptoPP::Integer _priv;
  CryptoPP::Integer _pub;
  std::string _Astring;
  CryptoPP::SecByteBlock _key;  
  std::string _request;
  Response _response;
  TaskPtr _task;
  std::string _responseData;
  Server& _server;

  Session(Server& server);
  virtual ~Session();
  std::string_view buildReply(HEADER& header, std::atomic<STATUS>& status);
  bool processTask(const HEADER& header);
};
