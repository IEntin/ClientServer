/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Crypto.h"
#include "Runnable.h"

class Server;
using ServerPtr = std::shared_ptr<Server>;
using ServerWeakPtr = std::weak_ptr<Server>;

namespace fifo {

class FifoAcceptor : public Runnable {
  void run() override;
  bool start() override;
  void stop() override;
  std::tuple<HEADERTYPE, CryptoPP::SecByteBlock, std::string>
  unblockAcceptor();
  void removeFifoFiles();
  std::string_view _acceptorName;
  ServerWeakPtr _server;
 public:
  explicit FifoAcceptor(ServerPtr server);
  ~FifoAcceptor() override;
};

} // end of namespace fifo
