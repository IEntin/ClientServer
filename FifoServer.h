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

using FifoServerPtr = std::shared_ptr<class FifoServer>;

class FifoServer : public std::enable_shared_from_this<FifoServer>, Runnable {
  static std::string _fifoDirectoryName;
  static std::pair<COMPRESSORS, bool> _compression;
  std::string _fifoName;
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
  std::thread _thread;
  static std::vector<FifoServerPtr> _fifoThreads;
  static void removeFifoFiles();
 public:
  explicit FifoServer(const std::string& fifoName);
  ~FifoServer() override;
  static bool startThreads(const std::string& fifoDirName,
			   const std::string& fifoBaseNames,
			   const std::pair<COMPRESSORS, bool>& compression);
  static void joinThreads();
};

} // end of namespace fifo
