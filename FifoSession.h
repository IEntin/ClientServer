/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Runnable.h"
#include <vector>

using Response = std::vector<std::string>;

struct ServerOptions;

namespace fifo {

using FifoAcceptorPtr = std::shared_ptr<class FifoAcceptor>;

class FifoSession final : public std::enable_shared_from_this<FifoSession>,
  public RunnableT<FifoSession> {
  const ServerOptions& _options;
  std::string _clientId;
  std::string _fifoName;
  FifoAcceptorPtr _parent;
  int _fdRead = -1;
  int _fdWrite = -1;
  bool receiveRequest(std::vector<char>& message, HEADER& header);
  bool sendResponse(const Response& response);
  std::vector<char> _uncompressedRequest;
  void run() override;
  void checkCapacity() override;
  bool sendStatusToClient();
 public:
  FifoSession(const ServerOptions& options, std::string_view clientId, FifoAcceptorPtr server);
  ~FifoSession() override;
  bool start() override;
  void stop() override;
};

} // end of namespace fifo
