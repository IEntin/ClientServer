/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cryptopp/secblock.h>

#include "Runnable.h"

using ServerPtr = std::shared_ptr<class Server>;
using ServerWeakPtr = std::weak_ptr<Server>;

namespace fifo {

class FifoAcceptor : public Runnable {
  void run() override;
  bool start() override;
  void stop() override;
  std::tuple<HEADERTYPE, std::string, CryptoPP::SecByteBlock, std::string>
  unblockAcceptor();
  void removeFifoFiles();
  std::string_view _acceptorName;
  ServerWeakPtr _server;
 public:
  explicit FifoAcceptor(ServerPtr server);
  ~FifoAcceptor() override;
};

} // end of namespace fifo
