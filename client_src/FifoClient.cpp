/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Fifo.h"
#include "MemoryPool.h"
#include <atomic>
#include <cstring>
#include <fcntl.h>

namespace fifo {

FifoClient::FifoClient(const FifoClientOptions& options) :
  Client(options), _options(options), _fifoName(options._fifoName) {}

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
  std::vector<char>& buffer = MemoryPool::getSecondaryBuffer(comprSize + 1);
  if (!Fifo::readString(_fdRead, buffer.data(), comprSize)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  std::string_view received(buffer.data(), comprSize);
  std::ostream* pstream = _options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
						       << " received compressed" << std::endl;
    std::string_view dstView = Compression::uncompress(received, uncomprSize);
    if (dstView.empty()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
      return false;
    }
    stream << dstView; 
  }
  else {
    static auto& printOnce[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
						       << " received not compressed" << std::endl;
    stream << received;
  }
  return true;
}

bool FifoClient::processTask(const Batch& payload) {
  // keep vector capacity
  static Batch modified;
  static const size_t bufferSize = MemoryPool::getInitialBufferSize();
  if (_options._prepareOnce) {
    static bool done = preparePackage(payload, modified, bufferSize);
    if (!done) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
  }
  else if (!preparePackage(payload, modified, bufferSize))
    return false;
  for (const auto& chunk : modified) {
    close(_fdRead);
    _fdRead = -1;
    _fdWrite = open(_fifoName.c_str(), O_WRONLY);
    if (_fdWrite == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< _fifoName << '-' << std::strerror(errno) << std::endl;
      return false;
    }
    if (!Fifo::writeString(_fdWrite, chunk)) {
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

// For the test payload is unchanged in a loop.

bool FifoClient::run(const Batch& payload) {
  _fdWrite = -1;
  CloseFileDescriptor raiiw(_fdWrite);
  _fdRead = -1;
  CloseFileDescriptor raiir(_fdRead);
  unsigned numberTasks = 0;
  do {
    Chronometer chronometer(_options._timing, __FILE__, __LINE__, __func__, _options._instrStream);
    if (!processTask(payload))
      return false;
    // limit output file size
    if (++numberTasks == _options._maxNumberTasks)
      break;
  } while (_options._runLoop);
  return true;
}

} // end of namespace fifo
