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
// The size of the aggregate depends on the buffer size.

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
  unsigned long long requestIndex = 0;
  std::vector<char>& aggregated = _memoryPool.getPrimaryBuffer();
  std::vector<char>& single(_memoryPool.getSecondaryBuffer());
  size_t minimumCapacity = HEADER_SIZE + CONV_BUFFER_SIZE + 2 + 1;
  if (single.capacity() < minimumCapacity) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":Minimum size of the buffer is " << minimumCapacity << std::endl;
    return false;
  }
  size_t offset = 0;
  size_t maxSubtaskSize = _memoryPool.getInitialBufferSize();
  for (const auto& line : lines) {
    *single.data() = '[';
    auto [ptr, ec] = std::to_chars(single.data() + 1, single.data() + CONV_BUFFER_SIZE, requestIndex++);
    if (ec != std::errc()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-error translating number:" << requestIndex << std::endl;
      continue;
    }
    size_t singleOffset = ptr - single.data();
    *(single.data() + singleOffset) = ']';
    ++singleOffset;
    if (singleOffset + line.size() > single.capacity())
      single.reserve(singleOffset + line.size() + 1);
    std::move(line.data(), line.data() + line.size(), single.data() + singleOffset);
    singleOffset += line.size();
    if (offset + singleOffset < maxSubtaskSize - HEADER_SIZE || offset == 0) {
      if (maxSubtaskSize < singleOffset + HEADER_SIZE) {
	std::vector<char> vect(singleOffset + HEADER_SIZE);
	std::move(single.data(), single.data() + singleOffset, vect.data() + HEADER_SIZE);
	_task.emplace_back();
	_task.back().swap(vect);
      }
      else {
	std::move(single.data(), single.data() + singleOffset, aggregated.data() + HEADER_SIZE + offset);
	offset += singleOffset;
      }
    }
    else {
      _task.emplace_back(aggregated.data(), aggregated.data() + offset + HEADER_SIZE);
      if (single.size() + HEADER_SIZE > maxSubtaskSize) {
	std::vector<char> vect(singleOffset + HEADER_SIZE);
	std::move(single.data(), single.data() + singleOffset, vect.data() + HEADER_SIZE);
	_task.emplace_back();
	_task.back().swap(vect);
	offset = 0;
      }
      else {
	std::move(single.data(), single.data() + singleOffset, aggregated.data() + HEADER_SIZE);
	offset = singleOffset;
      }
    }
  }
  if (offset > 0)
    _task.emplace_back(aggregated.data(), aggregated.data() + offset + HEADER_SIZE);
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
