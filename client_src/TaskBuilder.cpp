/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"

TaskBuilder::TaskBuilder(const ClientOptions& options) :
  _sourceName(options._sourceName),
  _compressor(options._compressor),
  _diagnostics(options._diagnostics),
  _requestIndex(0),
  _nextIdSz(4) {
  _input.open(_sourceName, std::ios::binary);
  if(!_input)
    throw std::ios::failure("Error opening file");
}

TaskBuilder::~TaskBuilder() {}

void TaskBuilder::run() {
  try {
    while (true) {
      if (!createTask()) {
	CERR << __FILE__ << ':' << __LINE__ << ' '
          << __func__ << ":failed.\n";
	return;
      }
      if (_state == TaskBuilderState::TASKDONE) {
	_promise2.set_value();
	return;
      }
      _promise2.get_future().get();
      std::promise<void>().swap(_promise2);
    }
  }
  catch (std::future_error& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ':' << e.what() << std::endl;
  }
  catch (std::system_error& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ':' << e.what() << std::endl;
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "-exception caught.\n";
  }
}

bool TaskBuilder::start() {
  return true;
}

void TaskBuilder::stop() {}

TaskBuilderState TaskBuilder::getTask(std::vector<char>& task) {
  TaskBuilderState result = TaskBuilderState::NONE;
  try {
    std::future<void> future = _promise1.get_future();
    future.get();
    if (_state == TaskBuilderState::ERROR) {
      std::vector<char>().swap(_task);
      std::vector<char>().swap(task);
    }
    else
      _task.swap(task);
    result = _state;
    if (_state == TaskBuilderState::SUBTASKDONE) {
      // promise is one-shot object, create a new one.
      std::promise<void>().swap(_promise1);
      _promise2.set_value();
    }
  }
  catch (std::future_error& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ':' << e.what() << std::endl;
    return TaskBuilderState::ERROR;
  }
  return result;
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
  _task.clear();
  ssize_t subtaskPos = 0;
  thread_local static std::vector<char> aggregate(MemoryPool::getExpectedSize());
  ssize_t aggregateSize = 0;
  // rough estimate for id and header to avoid reallocation.
  ssize_t maxSubtaskSize = MemoryPool::getExpectedSize() * 0.9;
  thread_local static std::string line;
  try {
    while (std::getline(_input, line)) {
      subtaskPos += line.size() + 1;
      aggregate.reserve(HEADER_SIZE + aggregateSize + _nextIdSz + line.size() + 1);
      int copied = copyRequestWithId(aggregate.data() + aggregateSize + HEADER_SIZE, line);
      aggregateSize += copied;
      if (subtaskPos >= maxSubtaskSize) {
	subtaskPos = 0;
	bool alldone = _input.peek() == std::istream::traits_type::eof();
	return compressSubtask(aggregate.data(), aggregate.data() + aggregateSize + HEADER_SIZE, alldone);
      }
      else
	continue;
    }
    if (subtaskPos)
      return compressSubtask(aggregate.data(), aggregate.data() + aggregateSize + HEADER_SIZE, true);
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ' ' << e.what() <<std::endl;
    _state = TaskBuilderState::ERROR;
    return false;
  }
  return true;
}

// Compress requests if options require.
// Generate header for every aggregated group of requests.

bool TaskBuilder::compressSubtask(char* beg, char* end, bool alldone) {
  bool bcompressed = _compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    CLOG << "compression " << (bcompressed ? "enabled" : "disabled")
      << std::endl;
  std::string_view uncompressed(beg + HEADER_SIZE, end);
  size_t uncomprSize = uncompressed.size();
  if (bcompressed) {
    std::string_view compressed = Compression::compress(uncompressed);
    // LZ4 may generate compressed larger than uncompressed.
    // In this case an uncompressed subtask is sent.
    if (compressed.size() >= uncomprSize) {
      encodeHeader(beg, uncomprSize, uncomprSize, COMPRESSORS::NONE, _diagnostics, 0, 'R');
      _task.assign(beg, end);
    }
    else {
      encodeHeader(beg, uncomprSize, compressed.size(), _compressor, _diagnostics, 0, 'R');
      std::copy(compressed.data(), compressed.data() + compressed.size(), beg + HEADER_SIZE);
      _task.assign(beg, beg + HEADER_SIZE + compressed.size());
    }
  }
  else {
    encodeHeader(beg, uncomprSize, uncomprSize, _compressor, _diagnostics, 0, 'R');
    _task.assign(beg, end);
  }
  try {
    if (_state != TaskBuilderState::ERROR)
      _state = alldone ? TaskBuilderState::TASKDONE : TaskBuilderState::SUBTASKDONE;
    _promise1.set_value();
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
             << ':' << e.what() << '\n';
    _state = TaskBuilderState::ERROR;
    throw;
    return false;
  }
  return true;
}
