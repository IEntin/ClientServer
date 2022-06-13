/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <fstream>

TaskBuilder::TaskBuilder(const ClientOptions& options,
			 MemoryPool& memoryPool,
			 ssize_t pos,
			 ssize_t  startRequestIndex,
			 int nextIdSz) :
  _sourceName(options._sourceName),
  _compressor(options._compressor),
  _diagnostics(options._diagnostics),
  _memoryPool(memoryPool),
  _sourcePos(pos),
  _requestIndex(startRequestIndex),
  _nextIdSz(nextIdSz) {}

TaskBuilder::~TaskBuilder() {}

void TaskBuilder::run() noexcept {
  try {
    _task.clear();
    if (!createTask()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      _done = false;
      _promise.set_value();
      return;
    }
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

bool TaskBuilder::getTask(std::vector<char>& task) {
  try {
    std::future<void> future = _promise.get_future();
    future.get();
    if (_done)
      _task.swap(task);
    else {
      std::vector<char>().swap(_task);
      std::vector<char>().swap(task);
    }
  }
  catch (std::future_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
             << "-exception:" << e.what() << std::endl;
    return false;
  }
  return _done;
}

int TaskBuilder::copyRequestWithId(char* dst, std::string_view line) {
  *dst = '[';
  auto [ptr, ec] = std::to_chars(dst + 1, dst + CONV_BUFFER_SIZE + 1, _requestIndex++);
  if (ec != std::errc())
    throw std::runtime_error(std::string("error translating number:") + std::to_string(_requestIndex));
  *ptr = ']';
  int offset = ptr - dst + 1;
  _nextIdSz = offset + 1;
  std::copy(line.data(), line.data() + line.size(), dst + offset);
  offset += line.size();
  *(dst + offset) = '\n';
  return ++offset;
}

// Read requests from the source, generate id for each.
// The source can be a file or a message received from
// another device.
// Aggregate requests to send them in one shot
// to reduce the number of write/read system calls.
// The size of the aggregate depends on the configured buffer size.

bool TaskBuilder::createTask() {
  if (_requestIndex == 0)
    _nextIdSz = 4;
  std::vector<char>& aggregate = _memoryPool.getSecondaryBuffer();
  long aggregateSize = 0;
  size_t maxSubtaskSize = _memoryPool.getInitialBufferSize();
  static std::string line;
  bool unfinished = true;
  if (_sourcePos > 1)
    _sourcePos -= 2;
  try {
    std::ifstream input(_sourceName, std::ios::binary);
    input.seekg(_sourcePos, std::ios_base::beg);
    while (input) {
      line.clear();
      std::getline(input, line);
      if (!input) {
	break;
      }
      _sourcePos += line.size() + 1;
      // in case aggregate is too small for a single line,
      // does nothing if configured buffer size is reasonable.
      aggregate.reserve(HEADER_SIZE + _nextIdSz + line.size() + 1);
      if (aggregateSize + HEADER_SIZE + _nextIdSz + line.size() + 1 < maxSubtaskSize || aggregateSize == 0) {
	int copied = copyRequestWithId(aggregate.data() + aggregateSize + HEADER_SIZE, line);
	aggregateSize += copied;
	unfinished = true;
	continue;
      }
      else {
	_sourcePos -= (static_cast<long>(line.size()) - 1);
	return compressSubtask(aggregate.data(), aggregate.data() + HEADER_SIZE + aggregateSize);
      }
    }
    if (unfinished)
      return compressSubtask(aggregate.data(), aggregate.data() + aggregateSize + HEADER_SIZE);
  }
  catch (const std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << e.what() <<std::endl;
    return false;
  }
  return true;
}

// Compress requests if options require.
// Generate header for every aggregated group of requests.

bool TaskBuilder::compressSubtask(char* beg, char* end) {
  bool bcompressed = _compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    std::clog << "compression " << (bcompressed ? "enabled" : "disabled") << std::endl;
  std::string_view uncompressed(beg + HEADER_SIZE, end);
  size_t uncomprSize = uncompressed.size();
  if (bcompressed) {
    std::string_view compressed = Compression::compress(uncompressed, _memoryPool);
    if (compressed.empty())
      return false;
    // LZ4 may generate compressed larger than uncompressed.
    // In this case an uncompressed subtask is sent.
    if (compressed.size() >= uncomprSize) {
      encodeHeader(beg, uncomprSize, uncomprSize, COMPRESSORS::NONE, _diagnostics);
      _task.assign(beg, end);
    }
    else {
      encodeHeader(beg, uncomprSize, compressed.size(), _compressor, _diagnostics);
      std::copy(compressed.data(), compressed.data() + compressed.size(), beg + HEADER_SIZE);
      _task.assign(beg, beg + HEADER_SIZE + compressed.size());
    }
  }
  else {
    encodeHeader(beg, uncomprSize, uncomprSize, _compressor, _diagnostics);
    _task.assign(beg, end);
  }
  try {
    _done = true;
    _promise.set_value();
  }
  catch (std::future_error& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
             << "-exception:" << e.what() << std::endl;
    return false;
  }
  return true;
}
