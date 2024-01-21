/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <vector>

#include <cryptopp/integer.h>

#include "Runnable.h"

using Response = std::vector<std::string>;
using TaskPtr = std::shared_ptr<class Task>;

namespace fifo {

class FifoSession final : public RunnableT<FifoSession> {
  Response _response;
  std::string _request;
  std::string _clientId;
  std::string _fifoName;
  TaskPtr _task;
  CryptoPP::Integer _priv;
  CryptoPP::Integer _pub;
  std::string _Astring;
  CryptoPP::SecByteBlock _key;
  bool receiveRequest(HEADER& header);
  bool sendResponse();
  bool start() override;
  void run() override;
  void stop() override;
  bool sendStatusToClient() override;
  std::string_view getId() override { return _clientId; }
  std::string_view getDisplayName() const override{ return "fifo"; }
  bool receiveBString();
 public:
  FifoSession();
  ~FifoSession() override;
};

} // end of namespace fifo
