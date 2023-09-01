/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <vector>

using Response = std::vector<std::string>;

namespace fifo {

class FifoSession final : public RunnableT<FifoSession> {
  std::string _clientId;
  std::string _fifoName;
  std::string _request;
  Response _response;
  static inline std::string_view _displayType = "fifo";
  bool receiveRequest(HEADER& header);
  bool sendResponse();
  bool start() override;
  void run() override;
  void stop() override;
  bool sendStatusToClient() override;
  std::string_view getId() override { return _clientId; }
 public:
  FifoSession();
  ~FifoSession() override;
};

} // end of namespace fifo
