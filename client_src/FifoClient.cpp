/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "Chronometer.h"
#include "Compression.h"
#include "Fifo.h"
#include "MemoryPool.h"
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
  if (!readBatch(fd, uncomprSize, comprSize, compressor == LZ4, dataStream)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return true;
}

bool readBatch(int fd, size_t uncomprSize, size_t comprSize, bool bcompressed, std::ostream* pstream) {
  std::vector<char>& buffer = MemoryPool::getSecondaryBuffer(comprSize + 1);
  if (!Fifo::readString(fd, buffer.data(), comprSize)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  std::string_view received(buffer.data(), comprSize);
  std::ostream& stream = pstream ? *pstream : std::cout;
  if (bcompressed) {
    std::string_view dstView = Compression::uncompress(received, uncomprSize);
    if (dstView.empty()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
      return false;
    }
    stream << dstView; 
  }
  else
    stream << received;
  return true;
}

bool preparePackage(const Batch& payload, Batch& modified) {
  modified.clear();
  // keep vector capacity
  static Batch aggregated;
  aggregated.clear();
  if (!mergePayload(payload, aggregated)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  if (!(buildMessage(aggregated, modified))) {
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
    static bool done[[maybe_unused]] = preparePackage(payload, modified);
    if (!done) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      return false;
    }
  }
  else if (!preparePackage(payload, modified))
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

// reduce number of write/read system calls.
bool mergePayload(const Batch& batch, Batch& aggregatedBatch) {
  if (batch.empty())
    return false;
  static const size_t BUFFER_SIZE = ProgramOptions::get("DYNAMIC_BUFFER_SIZE", 200000);
  std::string bigString;
  size_t reserveSize = 0;
  if (reserveSize)
    bigString.reserve(reserveSize + 1);
  for (std::string_view line : batch) {
    if (bigString.size() + line.size() < BUFFER_SIZE || bigString.empty())
      bigString.append(line);
    else {
      reserveSize = std::max(reserveSize, bigString.size());
      aggregatedBatch.push_back(std::move(bigString));
      bigString.clear();
      bigString.assign(line);
    }
  }
  if (!bigString.empty())
    aggregatedBatch.push_back(std::move(bigString));
  return true;
}

bool buildMessage(const Batch& payload, Batch& message) {
  if (payload.empty())
    return false;
  for (std::string_view str : payload) {
    char array[HEADER_SIZE] = {};
    static const auto[compressor, enabled] = Compression::isCompressionEnabled();
    size_t uncomprSize = str.size();
    message.emplace_back();
    if (enabled) {
      std::string_view dstView = Compression::compress(str);
      if (dstView.empty())
	return false;
      utility::encodeHeader(array, uncomprSize, dstView.size(), compressor);
      message.back().reserve(HEADER_SIZE + dstView.size() + 1);
      message.back().append(array, HEADER_SIZE).append(dstView);
    }
    else {
      utility::encodeHeader(array, uncomprSize, uncomprSize, compressor);
      message.back().reserve(HEADER_SIZE + str.size() + 1);
      message.back().append(array, HEADER_SIZE).append(str);
    }
  }
  return true;
}

std::string createIndexPrefix(size_t index) {
  static const std::string error = "Error";
  std::string indexStr;
  char arr[CONV_BUFFER_SIZE] = {};
  if (auto [ptr, ec] = std::to_chars(arr, arr + CONV_BUFFER_SIZE, index);
      ec == std::errc())
    return indexStr.append(1, '[').append(arr, ptr - arr).append(1, ']');
  else {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-error translating number" << std::endl;
    return error;
  }
}

void stop() {
  stopFlag.store(true);
}

} // end of namespace fifo
