/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"

using ServerWeakPtr = std::weak_ptr<class Server>;

namespace fifo {

class FifoAcceptor : public Runnable {
  void run() override;
  bool start() override;
  void stop() override;
  std::tuple<HEADERTYPE,
	     std::vector<unsigned char>,
	     std::vector<unsigned char>,
	     std::vector<unsigned char>>
  unblockAcceptor();
  void removeFifoFiles();
  std::string_view _acceptorName;
  ServerWeakPtr _server;
  HEADER _header;
 public:
  explicit FifoAcceptor(ServerWeakPtr server);
  ~FifoAcceptor() override;
};

} // end of namespace fifo
