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
  _options(options), _subtasks(_options._expectedMaxNumberSubtasksInTask) {
  _input.open(_options._sourceName, std::ios::binary);
  if(!_input)
    throw std::ios::failure("Error opening file");
}

TaskBuilder::~TaskBuilder() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

void TaskBuilder::run() {
  try {
    while (true) {
      if (createSubtask() != TaskBuilderState::SUBTASKDONE)
	break;
    }
  }
  catch (std::future_error& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
  }
  catch (std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
  }
  catch (...) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << "-exception caught.\n";
  }
}

unsigned TaskBuilder::getNumberObjects() const {
  return _objectCounter._numberObjects;
}

bool TaskBuilder::start() {
  return true;
}

void TaskBuilder::stop() {}

TaskBuilderState TaskBuilder::getTask(std::vector<char>& task) {
  auto& subtask = _subtasks[_subtaskConsumeIndex++];
  try {
    auto future = subtask._promise.get_future();
    future.get();
    switch (subtask._state) {
    case TaskBuilderState::SUBTASKDONE:
    case TaskBuilderState::TASKDONE:
      subtask._chars.swap(task);
      break;
    default:
      break;
    }
  }
  catch (const std::future_error& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
    return TaskBuilderState::ERROR;
  }
  return subtask._state;
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
// Aggregate requests for sending many in one shot
// to reduce the number of write/read system calls.
// The size of the aggregate depends on the configured buffer size.

TaskBuilderState TaskBuilder::createSubtask() {
  auto& subtask = _subtasks[_subtaskProduceIndex++];
  thread_local static std::vector<char> aggregate(MemoryPool::getExpectedSize());
  size_t aggregateSize = 0;
  // rough estimate for id and header to avoid reallocation.
  size_t maxSubtaskSize = MemoryPool::getExpectedSize() * 0.9;
  thread_local static std::string line;
  try {
    while (std::getline(_input, line)) {
      aggregate.reserve(HEADER_SIZE + aggregateSize + _nextIdSz + line.size() + 1);
      int copied = copyRequestWithId(aggregate.data() + aggregateSize + HEADER_SIZE, line);
      aggregateSize += copied;
      bool alldone = _input.peek() == std::istream::traits_type::eof();
      if (aggregateSize >= maxSubtaskSize || alldone) {
	return compressSubtask(subtask, aggregate.data(), aggregate.data() + aggregateSize + HEADER_SIZE, alldone);
      }
      else
	continue;
    }
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
    subtask._state = TaskBuilderState::ERROR;
    return TaskBuilderState::ERROR;
  }
  return TaskBuilderState::NONE;
}

// Compress requests if options require.
// Generate header for every aggregated group of requests.

TaskBuilderState TaskBuilder::compressSubtask(Subtask& subtask, char* beg, char* end, bool alldone) {
  struct SatisfyPromise {
    SatisfyPromise(Subtask& subtask) : _subtask(subtask) {}
    ~SatisfyPromise() {
      try {
	_subtask._promise.set_value();
      }
      catch (const std::exception& e) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << '\n';
	_subtask._state = TaskBuilderState::ERROR;
      }
    }
    Subtask& _subtask;
  } satisfyPromise(subtask);
  bool bcompressed = _options._compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    CLOG << "compression " << (bcompressed ? "enabled" : "disabled") << std::endl;
  std::string_view uncompressed(beg + HEADER_SIZE, end);
  size_t uncomprSize = uncompressed.size();
  if (bcompressed) {
    std::string_view compressed = Compression::compress(uncompressed);
    // LZ4 may generate compressed larger than uncompressed.
    // In this case an uncompressed subtask is sent.
    if (compressed.size() >= uncomprSize) {
      encodeHeader(beg, HEADERTYPE::REQUEST, uncomprSize, uncomprSize, COMPRESSORS::NONE, _options._diagnostics, 0);
      subtask._chars.assign(beg, end);
    }
    else {
      encodeHeader(beg, HEADERTYPE::REQUEST, uncomprSize, compressed.size(), _options._compressor, _options._diagnostics, 0);
      std::copy(compressed.data(), compressed.data() + compressed.size(), beg + HEADER_SIZE);
      subtask._chars.assign(beg, beg + HEADER_SIZE + compressed.size());
    }
  }
  else {
    encodeHeader(beg, HEADERTYPE::REQUEST, uncomprSize, uncomprSize, _options._compressor, _options._diagnostics, 0);
    subtask._chars.assign(beg, end);
  }
  if (subtask._state != TaskBuilderState::ERROR)
    subtask._state = alldone ? TaskBuilderState::TASKDONE : TaskBuilderState::SUBTASKDONE;
  return subtask._state;
}
