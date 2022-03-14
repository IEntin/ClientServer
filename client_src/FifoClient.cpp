/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Fifo.h"
#include "MemoryPool.h"
#include <atomic>
#include <cstring>
#include <iostream>
#include <fcntl.h>

namespace fifo {

FifoClient::FifoClient(const FifoClientOptions& options) :
  Client(options._bufferSize), _options(options), _fifoName(options._fifoName) {}

FifoClient::~FifoClient() {
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool FifoClient::receive() {
  auto [uncomprSize, comprSize, compressor, diagnostics, headerDone] = Fifo::readHeader(_fdRead);
  if (!headerDone)
    return false;
  if (!readBatch(uncomprSize, comprSize, compressor == COMPRESSORS::LZ4)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return true;
}

bool FifoClient::readBatch(size_t uncomprSize, size_t comprSize, bool bcompressed) {
  std::vector<char>& buffer = _memoryPool.getSecondaryBuffer(comprSize + 1);
  if (!Fifo::readString(_fdRead, buffer.data(), comprSize)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return printReply(_options, buffer, uncomprSize, comprSize, bcompressed);
}

bool FifoClient::processTask() {
  for (const auto& subtask : _task) {
    close(_fdRead);
    _fdRead = -1;
    _fdWrite = open(_fifoName.c_str(), O_WRONLY);
    if (_fdWrite == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< _fifoName << '-' << std::strerror(errno) << std::endl;
      return false;
    }
    if (!Fifo::writeString(_fdWrite, std::string_view(subtask.data(), subtask.size()))) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
    close(_fdWrite);
    _fdWrite = -1;
    _fdRead = open(_fifoName.c_str(), O_RDONLY);
    if (_fdRead == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< _fifoName << '-' << std::strerror(errno) << std::endl;
      return false;
    }
    if (!receive())
      return false;
  }
  return true;
}

bool FifoClient::run() {
  _fdWrite = -1;
  CloseFileDescriptor raiiw(_fdWrite);
  _fdRead = -1;
  CloseFileDescriptor raiir(_fdRead);
  return loop(_options);
}

} // end of namespace fifo
