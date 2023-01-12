/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Runnable.h"
#include <vector>

using Response = std::vector<std::string>;

struct ServerOptions;

class SessionContainer;

namespace fifo {

class FifoSession final : public std::enable_shared_from_this<FifoSession>,
  public RunnableT<FifoSession> {
  const ServerOptions& _options;
  SessionContainer& _sessionContainer;
  std::string _clientId;
  std::string _fifoName;
  bool receiveRequest(std::vector<char>& message, HEADER& header);
  bool sendResponse(const Response& response);
  std::vector<char> _uncompressedRequest;
  void run() override;
  void checkCapacity() override;
  bool sendStatusToClient();
 public:
  FifoSession(const ServerOptions& options,
	      std::string_view clientId,
	      SessionContainer& sessionContainer);
  ~FifoSession() override;
  bool start() override;
  void stop() override;
  void notify() override;
};

} // end of namespace fifo
