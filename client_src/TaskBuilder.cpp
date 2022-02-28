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
    Batch requestBatch;
    if (!createRequestBatch(requestBatch)) {
      _promise.set_value();
      return;
    }
    buildTask(requestBatch);
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
}

void TaskBuilder::getTask(Batch& task) {
  std::future<void> future = _promise.get_future();
  future.get();
  _task.swap(task);
}

bool TaskBuilder::buildTask(const Batch& payload) {
  static const size_t bufferSize = MemoryPool::getInitialBufferSize();
  // keep vector capacity
  static Batch aggregated;
  aggregated.clear();
  if (!mergePayload(payload, aggregated, bufferSize)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  if (!(buildMessage(aggregated, _task))) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return true;
}

// reduce number of write/read system calls.
bool TaskBuilder::mergePayload(const Batch& batch, Batch& aggregatedBatch, size_t bufferSize) {
  if (batch.empty())
    return false;
  std::string bigString;
  size_t reserveSize = 0;
  if (reserveSize)
    bigString.reserve(reserveSize + 1);
  for (std::string_view line : batch) {
    if (bigString.size() + line.size() < bufferSize || bigString.empty())
      bigString.append(line);
    else {
      reserveSize = std::max(reserveSize, bigString.size());
      aggregatedBatch.push_back(std::move(bigString));
      bigString.assign(std::move(line));
    }
  }
  if (!bigString.empty())
    aggregatedBatch.push_back(std::move(bigString));
  return true;
}

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

bool TaskBuilder::createRequestBatch(Batch& payload) {
  std::ifstream input(_sourceName, std::ifstream::in | std::ifstream::binary);
  if (!input) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' '
	      << strerror(errno) << ' ' << _sourceName << std::endl;
    return false;
  }
  unsigned long long requestIndex = 0;
  std::string line;
  Batch batch;
  while (std::getline(input, line)) {
    if (line.empty())
      continue;
    std::string taskLine(createRequestId(requestIndex++));
    taskLine.append(line.append(1, '\n'));
    payload.emplace_back(std::move(taskLine));
  }
  return true;
}

std::string TaskBuilder::createRequestId(size_t index) {
  char arr[CONV_BUFFER_SIZE + 1] = { '[' };
  auto [ptr, ec] = std::to_chars(arr + 1, arr + CONV_BUFFER_SIZE, index);
  assert(ec == std::errc() && ptr - arr < CONV_BUFFER_SIZE);
  *ptr = ']';
  return arr;
}
