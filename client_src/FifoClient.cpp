/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "Chronometer.h"
#include "Compression.h"
#include "Fifo.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <atomic>
#include <cstring>
#include <fcntl.h>

namespace fifo {

bool receive(int fd, std::ostream* dataStream) {
  auto [uncomprSize, comprSize, compressor, diagnostics, headerDone] = Fifo::readHeader(fd);
  if (!headerDone)
    return false;
  if (!Fifo::readBatch(fd, uncomprSize, comprSize, compressor == COMPRESSORS::LZ4, dataStream)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return true;
}

bool processTask(const Batch& payload,
		 const FifoClientOptions& options,
		 int& fdWrite,
		 int& fdRead,
		 std::ostream* dataStream) {
  // keep vector capacity
  static Batch modified;
  static const size_t bufferSize = MemoryPool::getInitialBufferSize();
  if (options._prepareOnce) {
    static bool done = utility::preparePackage(payload, modified, bufferSize, options._diagnostics);
    if (!done) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
  }
  else if (!utility::preparePackage(payload, modified, bufferSize, options._diagnostics))
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
    std::ostream* pstream = dataStream ? dataStream : options._dataStream;
    if (!receive(fdRead, pstream))
      return false;
  }
  return true;
}

// To run a test infinite loop must keep payload unchanged.
// in a real setup payload is used once and the vector is mutable.
bool run(const Batch& payload,
	 const FifoClientOptions& options,
	 std::ostream* dataStream) {
  int fdWrite = -1;
  CloseFileDescriptor raiiw(fdWrite);
  int fdRead = -1;
  CloseFileDescriptor raiir(fdRead);
  unsigned numberTasks = 0;
  do {
    Chronometer chronometer(options._timing, __FILE__, __LINE__, __func__, options._instrStream);
    if (!processTask(payload, options, fdWrite, fdRead, dataStream))
      return false;
    // limit output file size
    if (++numberTasks == options._maxNumberTasks)
      break;
  } while (options._runLoop);
  return true;
}

} // end of namespace fifo
