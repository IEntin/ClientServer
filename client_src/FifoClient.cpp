/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Fifo.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <iostream>

namespace fifo {

FifoClient::FifoClient(const FifoClientOptions& options) :
  Client(options), _fifoName(options._fifoName), _setPipeSize(options._setPipeSize) {}

FifoClient::~FifoClient() {
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool FifoClient::receive() {
  auto [uncomprSize, comprSize, compressor, diagnostics, headerDone] =
    Fifo::readHeader(_fdRead, _fifoName, _options._numberRepeatEINTR);
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
  if (!Fifo::readString(_fdRead, buffer.data(), comprSize, _fifoName, _options._numberRepeatEINTR)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return printReply(buffer, uncomprSize, comprSize, bcompressed);
}

bool FifoClient::processTask() {
  for (const auto& subtask : _task) {
    close(_fdRead);
    _fdRead = -1;
    int rep = 0;
    do {
      _fdWrite = open(_fifoName.data(), O_WRONLY | O_NONBLOCK);
      if (_fdWrite == -1 && (errno == ENXIO || errno == EINTR))
	std::this_thread::sleep_for(std::chrono::microseconds(_options._ENXIOwait));
    } while (_fdWrite == -1 && (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
    if (_fdWrite == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< std::strerror(errno) << ' ' << _fifoName << std::endl;
      return false;
    }
    if (_setPipeSize)
      Fifo::setPipeSize(_fdWrite, subtask.size());
    if (!Fifo::writeString(_fdWrite, std::string_view(subtask.data(), subtask.size()))) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
    close(_fdWrite);
    _fdWrite = -1;
    _fdRead = open(_fifoName.data(), O_RDONLY);
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
  utility::CloseFileDescriptor raiiw(_fdWrite);
  utility::CloseFileDescriptor raiir(_fdRead);
  return Client::run();
}

} // end of namespace fifo
