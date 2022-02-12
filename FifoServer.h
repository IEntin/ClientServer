/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <thread>
#include <vector>

using Batch = std::vector<std::string>;

enum class COMPRESSORS : unsigned short;

namespace fifo {

class FifoServer {
  static std::string _fifoDirectoryName;
  static std::pair<COMPRESSORS, bool> _compression;
  struct Runnable {
    explicit Runnable(const std::string& fifoName);
    ~Runnable();
    std::string _fifoName;
    int _fdRead = -1;
    int _fdWrite = -1;
    bool receiveRequest(std::vector<char>& batch, HEADER& header);
    bool sendResponse(Batch& response);
    std::vector<char> _uncompressedRequest;
    Batch _requestBatch;
    Batch _response;
    void operator()() noexcept;
  } _runnable;
  std::thread _thread;
  static std::vector<FifoServer> _fifoThreads;
  static void removeFifoFiles();
 public:
  explicit FifoServer(const std::string& fifoName);
  FifoServer(FifoServer&& other);
  ~FifoServer() = default;
  static bool startThreads(const std::string& fifoDirName,
			   const std::string& fifoBaseNames,
			   const std::pair<COMPRESSORS, bool>& compression);
  static void joinThreads();
};

} // end of namespace fifo
