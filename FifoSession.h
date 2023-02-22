/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPoolReference.h"
#include <vector>

using Response = std::vector<std::string>;
struct ServerOptions;
class ThreadPoolDiffObj;

namespace fifo {

class FifoSession final : public std::enable_shared_from_this<FifoSession>,
  public RunnableT<FifoSession> {
  const ServerOptions& _options;
  const std::string _fifoName;
  ThreadPoolReference _threadPool;
  bool receiveRequest(std::vector<char>& message, HEADER& header);
  bool sendResponse(const Response& response);
  std::vector<char> _uncompressedRequest;
  void run() override;
  bool start() override;
  void stop() override;
  void checkCapacity() override;
  bool sendStatusToClient() override;
 public:
  FifoSession(const ServerOptions& options, const std::string& clientId, ThreadPoolDiffObj& threadPool);
  ~FifoSession() override;
};

} // end of namespace fifo
