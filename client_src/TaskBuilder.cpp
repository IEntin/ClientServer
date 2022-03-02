#include "TaskBuilder.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

TaskBuilder::TaskBuilder(const std::string& sourceName, COMPRESSORS compressor, bool diagnostics) :
  _sourceName(sourceName), _compressor(compressor), _diagnostics(diagnostics) {
}

TaskBuilder::~TaskBuilder() {}

void TaskBuilder::run() noexcept {
  try {
    _task.clear();
    buildTask();
    _done = true;
    _promise.set_value();
  }
  catch (std::future_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception:" << e.what() << std::endl;
  }
  catch (std::system_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception:" << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-exception caught" << std::endl;
  }
  Batch testBatch;
}

void TaskBuilder::getTask(Batch& task) {
  std::future<void> future = _promise.get_future();
  future.get();
  _task.swap(task);
}

bool TaskBuilder::buildTask() {
  // keep vector capacity
  static Batch aggregatedBatch;
  aggregatedBatch.clear();
  createRequestBatch(aggregatedBatch);
  if (!(buildMessage(aggregatedBatch, _task))) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return true;
}

// Read requests from the source, generate id for each.
// The source can be a file or a message received from
// another device.
// Aggregaqte requests to send them in one shot
// to reduce the number of write/read system calls.
// The size of the aggregate depends on the buffer size.

bool TaskBuilder::createRequestBatch(Batch& aggregatedBatch) {
  std::ifstream input(_sourceName, std::ifstream::in | std::ifstream::binary);
  if (!input) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' '
	      << strerror(errno) << ' ' << _sourceName << std::endl;
    return false;
  }
  unsigned long long requestIndex = 0;
  size_t dstCapacity = MemoryPool::getInitialBufferSize();
  [[maybe_unused]] auto [aggregated, dummy] = MemoryPool::getPrimaryBuffer(dstCapacity);
  std::vector<char>& single(MemoryPool::getSecondaryBuffer(dstCapacity));
  size_t offset = 0;
  while (input) {
    std::memset(single.data(), '[', 1);
    auto [ptr, ec] = std::to_chars(single.data() + 1, single.data() + CONV_BUFFER_SIZE, requestIndex++);
    if (ec != std::errc()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-error translating number:" << requestIndex << std::endl;
      continue;
    }
    std::memset(ptr, ']', 1);
    input.getline(ptr + 1, dstCapacity - CONV_BUFFER_SIZE);
    std::streamsize numberRead = input.gcount();
    if (numberRead < 2)
      continue;
    // ignore terminating null
    --numberRead;
    std::memset(ptr + 1 + numberRead, '\n', 1);
    size_t size = ptr + 1 + numberRead + 1 - single.data();
    if (offset + size < dstCapacity || offset == 0) {
      std::memcpy(aggregated + offset, single.data(), size);
      offset += size;
    }
    else {
      aggregatedBatch.emplace_back(aggregated, offset);
      std::memcpy(aggregated, single.data(), size);
      offset = size;
    }
  }
  if (offset > 0)
    aggregatedBatch.emplace_back(aggregated, offset);
  return true;
}

// Compress requests if options require this.
// Generate header for every aggregated group of requests.

bool TaskBuilder::buildMessage(const Batch& payload, Batch& message) {
  if (payload.empty())
    return false;
  bool enabled = _compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    std::clog << "compression " << (enabled ? "enabled" : "disabled") << std::endl;
  for (std::string_view str : payload) {
    char array[HEADER_SIZE + 1] = {};
    size_t uncomprSize = str.size();
    message.emplace_back();
    if (enabled) {
      std::string_view dstView = Compression::compress(str);
      if (dstView.empty())
	return false;
      encodeHeader(array, uncomprSize, dstView.size(), _compressor, _diagnostics);
      message.back().reserve(HEADER_SIZE + dstView.size() + 1);
      message.back().append(array, HEADER_SIZE).append(dstView);
    }
    else {
      encodeHeader(array, uncomprSize, uncomprSize, _compressor, _diagnostics);
      message.back().reserve(HEADER_SIZE + str.size() + 1);
      message.back().append(array, HEADER_SIZE).append(str);
    }
  }
  return true;
}
