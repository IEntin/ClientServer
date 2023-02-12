/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Acceptor.h"

enum class HEADERTYPE : char;

namespace fifo {

class FifoAcceptor : public std::enable_shared_from_this<FifoAcceptor>,
  public Acceptor {
  void run() override;
  bool start() override;
  void stop() override;
  std::pair<HEADERTYPE, std::string> unblockAcceptor();
  void createSession();
  void removeFifoFiles();
  void openAcceptorWriteEnd();
 public:
  FifoAcceptor(const ServerOptions& options,
	       ThreadPoolBase& threadPoolAcceptor,
	       ThreadPoolDiffObj& threadPoolSession);
  ~FifoAcceptor() override;
};

} // end of namespace fifo
