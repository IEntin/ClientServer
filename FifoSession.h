/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPoolReference.h"
#include <vector>

using Response = std::vector<std::string>;
struct Options;
struct ServerOptions;
class ThreadPoolDiffObj;

namespace fifo {

class FifoSession final : public std::enable_shared_from_this<FifoSession>,
  public RunnableT<FifoSession> {
  const ServerOptions& _options;
  std::string _clientId;
  std::string _fifoName;
  ThreadPoolReference _threadPool;
  bool receiveRequest(std::vector<char>& message, HEADER& header);
   bool sendResponse(const Response& response);
  std::vector<char> _uncompressedRequest;
  int _fdReadS = -1;
  void run() override;
  bool start() override;
  void stop() override;
  void checkCapacity() override;
  bool sendStatusToClient() override;
 public:
  FifoSession(const ServerOptions& options,
	      std::string_view clientId,
	      ThreadPoolDiffObj& threadPool);
  ~FifoSession() override;
};

} // end of namespace fifo
