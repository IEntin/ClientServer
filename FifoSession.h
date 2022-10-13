/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "ObjectCounter.h"
#include "Runnable.h"
#include <vector>

using Response = std::vector<std::string>;

struct ServerOptions;

namespace fifo {

class FifoSession final : public Runnable {
  const ServerOptions& _options;
  const std::string _clientId;
  std::string _fifoName;
  RunnablePtr _parent;
  int _fdRead = -1;
  int _fdWrite = -1;
  bool receiveRequest(std::vector<char>& message, HEADER& header);
  bool sendResponse(const Response& response);
  std::vector<char> _uncompressedRequest;
  Response _response;
  ObjectCounter<FifoSession> _objectCounter;
  void run() override;
  void stop() override;
  unsigned getNumberObjects() const override;
  PROBLEMS checkCapacity() const override;
  bool sendStatusToClient(PROBLEMS problem);
 public:
  FifoSession(const ServerOptions& options, std::string_view clientId, RunnablePtr server);
  ~FifoSession() override;
  bool start() override;
};

} // end of namespace fifo
