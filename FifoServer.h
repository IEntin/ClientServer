/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "ThreadPool.h"
#include <memory>
#include <thread>
#include <vector>

using Batch = std::vector<std::string>;

enum class COMPRESSORS : unsigned short;

namespace fifo {

using FifoConnectionPtr = std::shared_ptr<class FifoConnection>;

using FifoServerPtr = std::shared_ptr<class FifoServer>;

class FifoConnection : public std::enable_shared_from_this<FifoConnection>, public Runnable {
  friend class FifoServer;
  bool receiveRequest(std::vector<char>& message, HEADER& header);
  bool readMsgBody(int fd,
		   size_t uncomprSize,
		   size_t comprSize,
		   bool bcompressed,
		   std::vector<char>& uncompressed);
  bool sendResponse(Batch& response);
  std::string _fifoName;
  FifoServerPtr _server;
  int _fdRead = -1;
  int _fdWrite = -1;
  std::vector<char> _uncompressedRequest;
  Batch _requestBatch;
  Batch _response;
 public:
  FifoConnection(const std::string& fifoName, FifoServerPtr server);
  ~FifoConnection() override;
  void run() noexcept override;
  void start();
  bool stop();
};

class FifoServer : public std::enable_shared_from_this<FifoServer> {
  friend class FifoConnection;
  const std::string _fifoDirName;
  std::pair<COMPRESSORS, bool> _compression;
  ThreadPool _threadPool;
  std::atomic<bool> _stopped = false;
  static FifoServerPtr _instance;
  bool startInstance(const std::vector<std::string>& fifoBaseNameVector);
  void stopInstance();
  bool stopped() const { return _stopped; }
  void removeFifoFiles();
  void passToThreadPool(FifoConnectionPtr connection);
 public:
  FifoServer(const std::string& fifoDirectoryName,
	     const std::pair<COMPRESSORS, bool>& compression,
	     size_t numberConnections);
  ~FifoServer() = default;
  static bool start(const std::string& fifoDirName,
		    const std::string& fifoBaseNames,
		    const std::pair<COMPRESSORS, bool>& compression);
  static void stop();
};

} // end of namespace fifo
