/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include <memory>
#include <thread>
#include <vector>

enum class COMPRESSORS : int;

using HEADER = std::tuple<ssize_t, ssize_t, COMPRESSORS, bool, bool>;

using Response = std::vector<std::string>;

struct ServerOptions;

namespace fifo {

using FifoConnectionPtr = std::shared_ptr<class FifoConnection>;

class FifoConnection : public Runnable {
  const ServerOptions& _options;
  std::string_view _fifoName;
  RunnablePtr _parent;
  int _fdRead = -1;
  int _fdWrite = -1;
  bool receiveRequest(std::vector<char>& message, HEADER& header);
  bool readMsgBody(int fd,
		   size_t uncomprSize,
		   size_t comprSize,
		   bool bcompressed,
		   std::vector<char>& uncompressed);
  bool sendResponse(const Response& response);
  std::vector<char> _uncompressedRequest;
  Response _response;
  void run() override;
  bool start() override;
  void stop() override;
 public:
  FifoConnection(const ServerOptions& options, std::string_view fifoName, RunnablePtr server);
  ~FifoConnection() override;
};

} // end of namespace fifo
