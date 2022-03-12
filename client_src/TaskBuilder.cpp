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
    if (!createRequests()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
      _done = false;
      _promise.set_value();
      return;
    }
    if (!(compressSubtasks())) {
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
  std::ifstream input(_sourceName, std::ifstream::in | std::ifstream::binary);
  if (!input) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' '
	      << strerror(errno) << ' ' << _sourceName << std::endl;
    return false;
  }
  unsigned long long requestIndex = 0;
  std::vector<char>& aggregated = MemoryPool::getPrimaryBuffer();
  std::vector<char>& single(MemoryPool::getSecondaryBuffer());
  size_t initialBufferSize = MemoryPool::getInitialBufferSize();
  size_t minimumCapacity = HEADER_SIZE + CONV_BUFFER_SIZE + 2 + 1;
  if (single.capacity() < minimumCapacity) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":Minimum size of the buffer is " << minimumCapacity << std::endl;
    return false;
  }
  size_t offset = 0;
  while (input) {
    std::memset(single.data(), '[', 1);
    auto [ptr, ec] = std::to_chars(single.data() + 1, single.data() + CONV_BUFFER_SIZE, requestIndex++);
    if (ec != std::errc()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-error translating number:" << requestIndex << std::endl;
      continue;
    }
    size_t singleOffset = ptr - single.data();
    std::memset(single.data() + singleOffset, ']', 1);
    ++singleOffset;
    // keep capacity
    static std::string line;
    line.clear();
    if (!std::getline(input, line, '\n'))
      break;
    if (singleOffset + line.size() > single.capacity())
      single.reserve(singleOffset + line.size() + 1);
    std::memcpy(single.data() + singleOffset, line.data(), line.size());
    singleOffset += line.size();
    std::memset(single.data() + singleOffset, '\n', 1);
    size_t size = singleOffset + 1;
    if (offset + size < initialBufferSize - HEADER_SIZE || offset == 0) {
      if (initialBufferSize < size + HEADER_SIZE) {
	std::vector<char> vect(size + HEADER_SIZE);
	std::memcpy(vect.data() + HEADER_SIZE, single.data(), size);
	_task.emplace_back();
	_task.back().swap(vect);
      }
      else {
	std::memcpy(aggregated.data() + HEADER_SIZE + offset, single.data(), size);
	offset += size;
      }
    }
    else {
      _task.emplace_back(aggregated.data(), aggregated.data() + offset + HEADER_SIZE);
      if (single.size() + HEADER_SIZE > initialBufferSize) {
	std::vector<char> vect(size + HEADER_SIZE);
	std::memcpy(vect.data() + HEADER_SIZE, single.data(), size);
	_task.emplace_back();
	_task.back().swap(vect);
	offset = 0;
      }
      else {
	std::memcpy(aggregated.data() + HEADER_SIZE, single.data(), size);
	offset = size;
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
  for (auto& item : _task) {
    std::string_view line(item.data() + HEADER_SIZE, item.data() + item.size());
    size_t uncomprSize = line.size();
    std::string_view viewIn(line.data(), line.size());
    if (bcompressed) {
      std::string_view dstView = Compression::compress(viewIn);
      if (dstView.empty())
	return false;
      if (dstView.size() >= viewIn.size()) {
	encodeHeader(item.data(), uncomprSize, uncomprSize, COMPRESSORS::NONE, _diagnostics);
	item.resize(HEADER_SIZE + viewIn.size());
      }
      else {
	encodeHeader(item.data(), uncomprSize, dstView.size(), _compressor, _diagnostics);
	std::memcpy(item.data() + HEADER_SIZE, dstView.data(), dstView.size());
	item.resize(HEADER_SIZE + dstView.size());
      }
    }
    else {
      encodeHeader(item.data(), uncomprSize, uncomprSize, _compressor, _diagnostics);
      item.resize(HEADER_SIZE + viewIn.size());
    }
  }
  return true;
}
