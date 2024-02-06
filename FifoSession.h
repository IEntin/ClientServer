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
  bool receiveRequest(HEADER& header);
  bool sendResponse();
  bool start() override;
  void run() override;
  void stop() override;
  bool sendStatusToClient() override;
  std::size_t getId() override { return _clientId; }
  std::string_view getDisplayName() const override{ return "fifo"; }
 public:
  FifoSession(ServerWeakPtr server, std::string_view Bstring);
  ~FifoSession() override;
};

} // end of namespace fifo
