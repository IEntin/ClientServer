/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <vector>

using Response = std::vector<std::string>;
using TaskPtr = std::shared_ptr<class Task>;

namespace fifo {

class FifoSession final : public RunnableT<FifoSession> {
  std::string _clientId;
  std::string _fifoName;
  std::string _request;
  Response _response;
  TaskPtr _task;
  bool receiveRequest(HEADER& header);
  bool sendResponse();
  bool start() override;
  void run() override;
  void stop() override;
  bool sendStatusToClient() override;
  std::string_view getId() override { return _clientId; }
  std::string_view getDisplayName() const override{ return "fifo"; }
 public:
  FifoSession();
  ~FifoSession() override;
};

} // end of namespace fifo
