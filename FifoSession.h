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

class FifoSession : public Runnable {
  const ServerOptions& _options;
  std::string _fifoName;
  RunnablePtr _parent;
  int _fdRead = -1;
  int _fdWrite = -1;
  unsigned short _ephemeralIndex;
  bool receiveRequest(std::vector<char>& message, HEADER& header);
  bool sendResponse(const Response& response);
  std::vector<char> _uncompressedRequest;
  Response _response;
  void run() override;
  bool start() override;
  void stop() override;
 public:
  FifoSession(const ServerOptions& options, unsigned short ephemeralIndex, RunnablePtr server);
  ~FifoSession() override;
};

} // end of namespace fifo
