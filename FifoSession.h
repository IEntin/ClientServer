/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "Session.h"

using ServerWeakPtr = std::weak_ptr<class Server>;

namespace fifo {

class FifoSession final : public RunnableT<FifoSession>,
			  public Session {
  std::string _fifoName;
  bool receiveRequest();
  bool sendResponse();
  void run() override;
  void stop() override;
  void displayCapacityCheck(std::atomic<unsigned>& totalNumberObjects) const override;
 public:
  FifoSession(ServerWeakPtr server,
	      std::span<unsigned char> msgHash,
	      std::span<unsigned char> pubB,
	      std::span<unsigned char> signatureWithPubKey);
  ~FifoSession() override;
  bool start() override;
  void sendStatusToClient() override;
};

} // end of namespace fifo

