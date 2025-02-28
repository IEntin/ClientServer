/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cryptopp/secblock.h>

#include "Runnable.h"
#include "Session.h"

class Server;
using ServerWeakPtr = std::weak_ptr<Server>;

namespace fifo {

class FifoSession final : public RunnableT<FifoSession>,
			  public Session {
  std::string _fifoName;
  bool receiveRequest();
  bool sendResponse();
  bool start() override;
  void run() override;
  void stop() override;
  void sendStatusToClient() override;
  void displayCapacityCheck(std::atomic<unsigned>& totalNumberObjects) const override;
 public:
  FifoSession(ServerWeakPtr server,
	      std::string_view msgHash,
	      const CryptoPP::SecByteBlock& pubB,
	      std::string_view signatureWithPubKey);
  ~FifoSession() override;
};

} // end of namespace fifo
