/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Runnable.h"
#include "ThreadPool.h"
#include <memory>
#include <thread>
#include <vector>

enum class COMPRESSORS : short unsigned int;

using HEADER = std::tuple<ssize_t, ssize_t, COMPRESSORS, bool, bool>;

using Batch = std::vector<std::string>;

using TaskControllerPtr = std::shared_ptr<class TaskController>;

struct ServerOptions;

namespace fifo {

using FifoServerPtr = std::shared_ptr<class FifoServer>;

using FifoConnectionPtr = std::shared_ptr<class FifoConnection>;

class FifoServer : public std::enable_shared_from_this<FifoServer> {
  TaskControllerPtr _taskController;
  const std::string _fifoDirName;
  const COMPRESSORS _compressor;
  ThreadPool _threadPool;
  std::atomic<bool> _stopped = false;
  void removeFifoFiles();
  std::vector<std::string> _fifoNames;
  void wakeupPipes();
 public:
  FifoServer(TaskControllerPtr taskController, const ServerOptions& options);
  ~FifoServer();
  bool start(const ServerOptions& options);
  void stop();
  bool stopped() const { return _stopped; }
  void pushToThreadPool(FifoConnectionPtr connection);
};

class FifoConnection : public Runnable {
  const ServerOptions& _options;
  TaskControllerPtr _taskController;
  const std::string_view _fifoName;
  COMPRESSORS _compressor;
  FifoServerPtr _server;
  int _fdRead = -1;
  int _fdWrite = -1;
  bool receiveRequest(std::vector<char>& message, HEADER& header);
  bool readMsgBody(int fd,
		   size_t uncomprSize,
		   size_t comprSize,
		   bool bcompressed,
		   std::vector<char>& uncompressed);
  bool sendResponse(Batch& response);
  std::vector<char> _uncompressedRequest;
  Batch _response;
  void run() noexcept override;
 public:
  FifoConnection(const ServerOptions& options,
		 TaskControllerPtr taskController,
		 std::string_view fifoName,
		 COMPRESSORS compressor,
		 FifoServerPtr server);
  ~FifoConnection() override;
};

} // end of namespace fifo
