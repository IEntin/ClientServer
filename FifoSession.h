/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "ThreadPoolReference.h"
#include <vector>

using Response = std::vector<std::string>;
struct ServerOptions;
class Server;
class ThreadPoolDiffObj;

namespace fifo {

class FifoSession final : public std::enable_shared_from_this<FifoSession>,
  public RunnableT<FifoSession> {
  const ServerOptions& _options;
  std::string _clientId;
  std::string _fifoName;
  ThreadPoolReference<ThreadPoolDiffObj> _threadPool;
  static inline std::string_view _name = "fifo";
  bool receiveRequest(HEADER& header);
  bool sendResponse(const Response& response);
  void run() override;
  bool start() override;
  void stop() override;
  bool sendStatusToClient() override;
 public:
  FifoSession(Server& server, std::string_view clientId);
  ~FifoSession() override;
};

} // end of namespace fifo
