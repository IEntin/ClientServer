/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"

TaskBuilder::TaskBuilder(const ClientOptions& options, MemoryPool& memoryPool) :
  _sourceName(options._sourceName),
  _compressor(options._compressor),
  _diagnostics(options._diagnostics),
  _memoryPool(memoryPool) {}

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

int TaskBuilder::copyRequestWithId(char* dst, std::string_view line, int& nextIdSz) {
  *dst = '[';
  auto [ptr, ec] = std::to_chars(dst + 1, dst + CONV_BUFFER_SIZE + 1, _requestIndex++);
  if (ec != std::errc())
    throw std::runtime_error(std::string("error translating number:") + std::to_string(_requestIndex));
  *ptr = ']';
  int offset = ptr - dst + 1;
  nextIdSz = offset + 1;
  std::move(line.data(), line.data() + line.size(), dst + offset);
  return offset + line.size();
}

// Read requests from the source, generate id for each.
// The source can be a file or a message received from
// another device.
// Aggregate requests to send them in one shot
// to reduce the number of write/read system calls.
// The size of the aggregate depends on the configured buffer size.

bool TaskBuilder::createTask() {
  static std::vector<std::string_view> lines;
  lines.clear();
  int nextIdSz = 4;
  try {
    static std::vector<char> buffer;
    utility::readFile(_sourceName, buffer);
    utility::split(buffer, lines, '\n', 1);
    std::vector<char>& aggregate = _memoryPool.getSecondaryBuffer();
    long aggregateSize = 0;
    long maxSubtaskSize = _memoryPool.getInitialBufferSize();
    for (auto&& line : lines) {
      // in case aggregate is too small for a single, does
      // nothing if configured buffer size is reasonable.
      aggregate.reserve(line.size() + HEADER_SIZE + nextIdSz);
      if (aggregateSize + static_cast<long>(line.size()) + nextIdSz < maxSubtaskSize - HEADER_SIZE || aggregateSize == 0) {
	int copied = copyRequestWithId(aggregate.data() + HEADER_SIZE + aggregateSize, line, nextIdSz);
	aggregateSize += copied;
      }
      else {
	compressSubtask(std::vector<char>(aggregate.data(), aggregate.data() + HEADER_SIZE + aggregateSize));
	int copied = copyRequestWithId(aggregate.data() + HEADER_SIZE, line, nextIdSz);
	aggregateSize = copied;
      }
    }
    compressSubtask(std::vector<char>(aggregate.data(), aggregate.data() + aggregateSize + HEADER_SIZE));
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

bool TaskBuilder::compressSubtask(std::vector<char>&& subtask) {
  bool bcompressed = _compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    std::clog << "compression " << (bcompressed ? "enabled" : "disabled") << std::endl;
  std::string_view uncompressed(subtask.data() + HEADER_SIZE, subtask.data() + subtask.size());
  size_t uncomprSize = uncompressed.size();
  if (bcompressed) {
    std::string_view compressed = Compression::compress(uncompressed, _memoryPool);
    if (compressed.empty())
      return false;
    // LZ4 may generate compressed larger than uncompressed.
    // In this case an uncompressed task is sent.
    if (compressed.size() >= uncomprSize) {
      encodeHeader(subtask.data(), uncomprSize, uncomprSize, COMPRESSORS::NONE, _diagnostics);
      _task.emplace_back(std::move(subtask));
    }
    else {
      encodeHeader(subtask.data(), uncomprSize, compressed.size(), _compressor, _diagnostics);
      std::move(compressed.data(), compressed.data() + compressed.size(), subtask.data() + HEADER_SIZE);
      _task.emplace_back(subtask.data(), subtask.data() + HEADER_SIZE + compressed.size());
    }
  }
  else {
    encodeHeader(subtask.data(), uncomprSize, uncomprSize, _compressor, _diagnostics);
    _task.emplace_back(std::move(subtask));
  }
  return true;
}
