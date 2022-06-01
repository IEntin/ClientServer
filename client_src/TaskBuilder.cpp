/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <cassert>
#include <cstring>

TaskBuilder::TaskBuilder(const ClientOptions& options, MemoryPool& memoryPool) :
  _sourceName(options._sourceName),
  _compressor(options._compressor),
  _diagnostics(options._diagnostics),
  _memoryPool(memoryPool) {}

TaskBuilder::~TaskBuilder() {}

void TaskBuilder::run() noexcept {
  try {
    _task.clear();
    if (!createRequests()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      _done = false;
      _promise.set_value();
      return;
    }
    if (!compressSubtasks()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      _done = false;
      _promise.set_value();
      return;
    }
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

bool TaskBuilder::getTask(Vectors& task) {
  std::future<void> future = _promise.get_future();
  future.get();
  if (_done)
    _task.swap(task);
  else {
    Vectors().swap(_task);
    Vectors().swap(task);
  }
  return _done;
}

// Read requests from the source, generate id for each.
// The source can be a file or a message received from
// another device.
// Aggregate requests to send them in one shot
// to reduce the number of write/read system calls.
// The size of the aggregate depends on the configured buffer size.

bool TaskBuilder::createRequests() {
  static std::vector<std::string_view> lines;
  lines.clear();
  try {
    static std::vector<char> buffer;
    utility::readFile(_sourceName, buffer);
    utility::split(buffer, lines, '\n', 1);
  }
  catch (const std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << e.what() <<std::endl;
    return false;
  }
  std::vector<char>& aggregate = _memoryPool.getPrimaryBuffer();
  std::vector<char>& single(_memoryPool.getSecondaryBuffer());
  size_t minimumCapacity = HEADER_SIZE + CONV_BUFFER_SIZE + 2 + 1;
  if (single.capacity() < minimumCapacity) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":Minimum size of the buffer is " << minimumCapacity << std::endl;
    return false;
  }
  size_t aggregateSize = 0;
  size_t maxSubtaskSize = _memoryPool.getInitialBufferSize();
  unsigned long long requestIndex = 0;
  for (const auto& line : lines) {
    *single.data() = '[';
    auto [ptr, ec] = std::to_chars(single.data() + 1, single.data() + CONV_BUFFER_SIZE, requestIndex++);
    if (ec != std::errc()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-error translating number:" << requestIndex << std::endl;
      continue;
    }
    size_t singleSize = ptr - single.data();
    *(single.data() + singleSize) = ']';
    ++singleSize;
    // in case single is too small for a line
    single.reserve(singleSize + line.size());
    std::move(line.data(), line.data() + line.size(), single.data() + singleSize);
    singleSize += line.size();
    // in case aggregate is too small for a single
    aggregate.reserve(singleSize + HEADER_SIZE);
    if (aggregateSize + singleSize < maxSubtaskSize - HEADER_SIZE || aggregateSize == 0) {
      std::move(single.data(), single.data() + singleSize, aggregate.data() + HEADER_SIZE + aggregateSize);
      aggregateSize += singleSize;
    }
    else {
      _task.emplace_back(aggregate.data(), aggregate.data() + aggregateSize + HEADER_SIZE);
      std::move(single.data(), single.data() + singleSize, aggregate.data() + HEADER_SIZE);
      aggregateSize = singleSize;
    }
  }
  _task.emplace_back(aggregate.data(), aggregate.data() + aggregateSize + HEADER_SIZE);
  return true;
}

// Compress requests if options require this.
// Generate header for every aggregated group of requests.

bool TaskBuilder::compressSubtasks() {
  if (_task.empty())
    return false;
  bool bcompressed = _compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    std::clog << "compression " << (bcompressed ? "enabled" : "disabled") << std::endl;
  for (auto& subtask : _task) {
    std::string_view uncompressed(subtask.data() + HEADER_SIZE, subtask.data() + subtask.size());
    size_t uncomprSize = uncompressed.size();
    if (bcompressed) {
      std::string_view compressed = Compression::compress(uncompressed, _memoryPool);
      if (compressed.empty())
	return false;
      // LZ4 may generate compressed larger than uncompressed.
      if (compressed.size() >= uncomprSize) {
	encodeHeader(subtask.data(), uncomprSize, uncomprSize, COMPRESSORS::NONE, _diagnostics);
	subtask.resize(HEADER_SIZE + uncomprSize);
      }
      else {
	encodeHeader(subtask.data(), uncomprSize, compressed.size(), _compressor, _diagnostics);
	std::move(compressed.data(), compressed.data() + compressed.size(), subtask.data() + HEADER_SIZE);
	subtask.resize(HEADER_SIZE + compressed.size());
      }
    }
    else {
      encodeHeader(subtask.data(), uncomprSize, uncomprSize, _compressor, _diagnostics);
      subtask.resize(HEADER_SIZE + uncomprSize);
    }
  }
  return true;
}
