/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Fifo.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <atomic>
#include <cstring>
#include <fcntl.h>

namespace fifo {

bool FifoClient::receive(int fd, std::ostream* dataStream) {
  auto [uncomprSize, comprSize, compressor, diagnostics, headerDone] = Fifo::readHeader(fd);
  if (!headerDone)
    return false;
  if (!readBatch(fd, uncomprSize, comprSize, compressor == COMPRESSORS::LZ4, dataStream)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return true;
}

bool FifoClient::readBatch(int fd,
			   size_t uncomprSize,
			   size_t comprSize,
			   bool bcompressed,
			   std::ostream* pstream) {
  std::vector<char>& buffer = MemoryPool::getSecondaryBuffer(comprSize + 1);
  if (!Fifo::readString(fd, buffer.data(), comprSize)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  std::string_view received(buffer.data(), comprSize);
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

bool FifoClient::processTask(const Batch& payload,
			     const FifoClientOptions& options,
			     int& fdWrite,
			     int& fdRead) {
  // keep vector capacity
  static Batch modified;
  static const size_t bufferSize = MemoryPool::getInitialBufferSize();
  if (options._prepareOnce) {
    static bool done = utility::preparePackage(payload, modified, bufferSize, options);
    if (!done) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
  }
  else if (!utility::preparePackage(payload, modified, bufferSize, options))
    return false;
  for (const auto& chunk : modified) {
    close(fdRead);
    fdRead = -1;
    fdWrite = open(options._fifoName.c_str(), O_WRONLY);
    if (fdWrite == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< options._fifoName << '-' << std::strerror(errno) << std::endl;
      return false;
    }
    if (!Fifo::writeString(fdWrite, chunk)) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
    close(fdWrite);
    fdWrite = -1;
    fdRead = open(options._fifoName.c_str(), O_RDONLY);
    if (fdRead == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		<< options._fifoName << '-' << std::strerror(errno) << std::endl;
      return false;
    }
    if (!receive(fdRead, options._dataStream))
      return false;
  }
  return true;
}

// To run a test infinite loop must keep payload unchanged.
// in a real setup payload is used once and the vector is mutable.
bool FifoClient::run(const Batch& payload, const FifoClientOptions& options) {
  int fdWrite = -1;
  CloseFileDescriptor raiiw(fdWrite);
  int fdRead = -1;
  CloseFileDescriptor raiir(fdRead);
  unsigned numberTasks = 0;
  do {
    Chronometer chronometer(options._timing, __FILE__, __LINE__, __func__, options._instrStream);
    if (!processTask(payload, options, fdWrite, fdRead))
      return false;
    // limit output file size
    if (++numberTasks == options._maxNumberTasks)
      break;
  } while (options._runLoop);
  return true;
}

} // end of namespace fifo
