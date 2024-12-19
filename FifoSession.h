/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

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
  std::string_view getDisplayName() const override{ return "fifo"; }
 public:
  FifoSession(ServerWeakPtr server,
	      const CryptoPP::SecByteBlock& pubB,
	      std::span<uint8_t> signatureWithPubKey);
  ~FifoSession() override;
};

} // end of namespace fifo
