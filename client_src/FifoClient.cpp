/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "Chronometer.h"
#include "Compression.h"
#include "Fifo.h"
#include "ProgramOptions.h"
#include "Utility.h"
#include <atomic>
#include <fcntl.h>

namespace fifo {

namespace {

volatile std::atomic<bool> stopFlag = false;

} // end of anonimous namespace

bool receive(int fd, std::ostream* dataStream) {
  auto [uncomprSize, comprSize, compressor, headerDone] = Fifo::readHeader(fd);
  if (!headerDone)
    return false;
  if (!Fifo::readBatch(fd, uncomprSize, comprSize, compressor == LZ4, dataStream)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return true;
}

bool processTask(const Batch& payload, int& fdWrite, int& fdRead, std::ostream* dataStream) {
  // keep vector capacity
  static Batch modified;
  // Simulate fast client to measure server performance accurately.
  // used with "RunLoop" : true
  static bool prepareOnce = ProgramOptions::get("PrepareBatchOnce", false);
  if (prepareOnce) {
    static bool done[[maybe_unused]] = utility::preparePackage(payload, modified);
    if (!done) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
  }
  else if (!utility::preparePackage(payload, modified))
    return false;
  static const std::string fifoDirectoryName = ProgramOptions::get("FifoDirectoryName", std::string());
  static const std::string fifoBaseName = ProgramOptions::get("FifoBaseName", std::string());
  static const std::string fifoName = fifoDirectoryName + '/' + fifoBaseName;
  for (const auto& chunk : modified) {
    close(fdRead);
    fdRead = -1;
    if (!stopFlag) {
      fdWrite = open(fifoName.c_str(), O_WRONLY);
      if (fdWrite == -1) {
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		  << fifoName << '-' << std::strerror(errno) << std::endl;
	return false;
      }
    }
    if (fdWrite == -1)
      return false;
    if (!Fifo::writeString(fdWrite, chunk)) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
    close(fdWrite);
    fdWrite = -1;
    if (!stopFlag) {
      fdRead = open(fifoName.c_str(), O_RDONLY);
      if (fdRead == -1) {
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
		  << fifoName << '-' << std::strerror(errno) << std::endl;
	return false;
      }
    }
    if (!receive(fdRead, dataStream))
      return false;
  }
  return true;
}

// client fifo loop
// to run a test infinite loop must keep payload unchanged.
// in a real setup payload is used once and vector is mutable.
bool run(const Batch& payload,
	 bool runLoop,
	 unsigned maxNumberTasks,
	 std::ostream* dataStream,
	 std::ostream* instrStream) {
  static const bool timing = ProgramOptions::get("Timing", false);
  int fdWrite = -1;
  CloseFileDescriptor raiiw(fdWrite);
  int fdRead = -1;
  CloseFileDescriptor raiir(fdRead);
  unsigned numberTasks = 0;
  do {
    Chronometer chronometer(timing, __FILE__, __LINE__, __func__, instrStream);
    if (!processTask(payload, fdWrite, fdRead, dataStream))
      return false;
    // limit output file size
    if (++numberTasks == maxNumberTasks)
      break;
  } while (runLoop);
  return true;
}

void stop() {
  stopFlag.store(true);
}

} // end of namespace fifo
