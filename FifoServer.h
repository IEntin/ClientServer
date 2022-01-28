/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <thread>
#include <vector>

using Batch = std::vector<std::string>;

namespace fifo {

class FifoServer {
  static std::vector<FifoServer> _runnables;
  static std::vector<std::thread> _threads;
  static std::string _fifoDirectoryName;
  std::string _fifoName;
  int _fdRead = -1;
  int _fdWrite = -1;
  bool receiveRequest(std::vector<char>& batch, HEADER& header);
  bool sendResponse(Batch& response);
public:
  explicit FifoServer(const std::string& receiveFifoName);
  ~FifoServer();
  void operator()() noexcept;
  static bool startThreads(const std::string& fifoDirName, const std::string& fifoBaseNames);
  static void joinThreads();
  static void removeFifoFiles();
  std::vector<char> _uncompressedRequest;
  Batch _requestBatch;
  Batch _response;
};

} // end of namespace fifo
