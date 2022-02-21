/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Compression.h"
#include "Header.h"
#include "ThreadPool.h"
#include <memory>
#include <thread>
#include <vector>

using Batch = std::vector<std::string>;

namespace fifo {

using FifoServerPtr = std::shared_ptr<class FifoServer>;

using FifoConnectionPtr = std::shared_ptr<class FifoConnection>;

class FifoServer : public std::enable_shared_from_this<FifoServer> {
  friend class FifoConnection;
  const std::string _fifoDirName;
  const CompressionDescription _compression;
  ThreadPool _threadPool;
  std::atomic<bool> _stopped = false;
  bool stopped() const { return _stopped; }
  void removeFifoFiles();
  std::vector<std::string> _fifoNames;
  static FifoServerPtr _instance;
  bool startInstance();
  void stopInstance();
  void wakeupPipes();
 public:
  FifoServer(const std::string& fifoDirName,
	     const std::vector<std::string>& fifoBaseNames,
	     const CompressionDescription& compression);
  ~FifoServer();
  static bool start(const std::string& fifoDirName,
		    const std::string& fifoBaseNames,
		    const CompressionDescription& compression);
  static void stop();
};

class FifoConnection : public std::enable_shared_from_this<FifoConnection>, public Runnable {
  friend class FifoServer;
  std::string _fifoName;
  FifoServerPtr _server;
  CompressionDescription _compression;
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
  Batch _requestBatch;
  Batch _response;
  void run() noexcept override;
  void start();
 public:
  explicit FifoConnection(const std::string& fifoName, FifoServerPtr server);
  ~FifoConnection() override;
};

} // end of namespace fifo
