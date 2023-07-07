/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <vector>

using Response = std::vector<std::string>;
struct ServerOptions;

namespace fifo {

class FifoSession final : public RunnableT<FifoSession> {
  const ServerOptions& _options;
  std::string _clientId;
  std::string _fifoName;
  static inline std::string_view _name = "fifo";
  bool receiveRequest(HEADER& header);
  bool sendResponse(const Response& response);
  bool start() override { return true; }
  void run() override;
  void stop() override;
  bool sendStatusToClient() override;
 public:
  FifoSession(const ServerOptions& options, std::string_view clientId);
  ~FifoSession() override;
};

} // end of namespace fifo
