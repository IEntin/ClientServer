/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include <vector>

using Response = std::vector<std::string>;

struct ServerOptions;

class Server;

namespace fifo {

class FifoSession final : public std::enable_shared_from_this<FifoSession>,
  public RunnableT<FifoSession> {
  const ServerOptions& _options;
  Server& _server;
  std::string _clientId;
  std::string _fifoName;
  bool receiveRequest(std::vector<char>& message, HEADER& header);
  bool sendResponse(const Response& response);
  std::vector<char> _uncompressedRequest;
  void run() override;
  bool start() override;
  void stop() override;
  bool notify() override;
  void checkCapacity() override;
  bool sendStatusToClient();
 public:
  FifoSession(const ServerOptions& options,
	      std::string_view clientId,
	      Server& server);
  ~FifoSession() override;
};

} // end of namespace fifo
