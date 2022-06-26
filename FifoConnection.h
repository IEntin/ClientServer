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

using TaskControllerPtr = std::shared_ptr<class TaskController>;

struct ServerOptions;

namespace fifo {

using FifoServerPtr = std::shared_ptr<class FifoServer>;

using FifoConnectionPtr = std::shared_ptr<class FifoConnection>;

class FifoConnection : public Runnable {
  const ServerOptions& _options;
  TaskControllerPtr _taskController;
  std::string_view _fifoName;
  std::atomic<int>& _numberConnections;
  std::atomic<bool>& _stopped;
  FifoServerPtr _server;
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
 public:
  FifoConnection(const ServerOptions& options,
		 TaskControllerPtr taskController,
		 std::string_view fifoName,
		 std::atomic<int>& numberConnections,
		 std::atomic<bool>& stopped,
		 FifoServerPtr server);
  ~FifoConnection() override;
};

} // end of namespace fifo
