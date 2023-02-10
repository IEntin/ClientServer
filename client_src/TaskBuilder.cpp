/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "MemoryPool.h"
#include "ThreadPoolBase.h"
#include "Utility.h"
#include <filesystem>

TaskBuilder::TaskBuilder(const ClientOptions& options, ThreadPoolBase& threadPool) :
  _options(options),
  _threadPool(threadPool) {
  _threadPool.numberRelatedObjects()++;
  _input.open(_options._sourceName, std::ios::binary);
  if(!_input)
    throw std::ios::failure("Error opening file");
  // rough estimate of a number of subtasks in task
  // assuming service info (id, header) is not larger
  // than half of content. The actual number of subtasks
  // is supposed to be smaller than this estimate.
  size_t sourceSize = std::filesystem::file_size(_options._sourceName);
  int expectedNumberSubtasks = (3 * sourceSize / 2 / _options._bufferSize + 1);
  _subtasks.resize(expectedNumberSubtasks);
}

TaskBuilder::~TaskBuilder() {
  _threadPool.numberRelatedObjects()--;
  Trace << std::endl;
}

void TaskBuilder::run() {
  try {
    while (true) {
      if (createSubtask() != TaskBuilderState::SUBTASKDONE)
	break;
    }
  }
  catch (std::exception& e) {
    LogError << e.what() << std::endl;
  }
  catch (...) {
    LogError << "exception caught." << std::endl;
  }
}

bool TaskBuilder::start() {
  return true;
}

void TaskBuilder::stop() {
  _stopped = true;
  std::vector<char> task;
  getTask(task);
}

TaskBuilderState TaskBuilder::getTask(std::vector<char>& task) {
  TaskBuilderState state = TaskBuilderState::NONE;
  if (_stopped)
    return TaskBuilderState::STOPPED;
  try {
    auto& subtask = _subtasks.at(_subtaskConsumeIndex);
    _subtaskConsumeIndex++;
    auto future = subtask._promise.get_future();
    future.get();
    state = subtask._state;
    switch (state) {
    case TaskBuilderState::SUBTASKDONE:
    case TaskBuilderState::TASKDONE:
      subtask._chars.swap(task);
      break;
    default:
      break;
    }
  }
  catch (const std::future_error& e) {
    LogError << e.what() << std::endl;
    return TaskBuilderState::ERROR;
  }
  catch (const std::out_of_range& e) {
    LogError << e.what()
      << ". Increase \"ExpectedMaxNumberSubtasksInTask\" in ClientOptions.json!" << std::endl;
    return TaskBuilderState::ERROR;
  }
  return state;
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
  thread_local static std::vector<char> aggregate(_options._bufferSize);
  size_t aggregateSize = 0;
  // rough estimate for id and header to avoid reallocation.
  size_t maxSubtaskSize = _options._bufferSize * 0.9;
  thread_local static std::string line;
  if (_subtaskProduceIndex >= _subtasks.size()) {
    LogError << "std::out_of_range avoided.\n"
	  << "Increase \"ExpectedMaxNumberSubtasksInTask\" in ClientOptions.json!" << std::endl;
    return TaskBuilderState::ERROR;
  }
  auto& subtask = _subtasks.at(_subtaskProduceIndex);
  _subtaskProduceIndex++;
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
    LogError << e.what() << std::endl;
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
	LogError << e.what() << std::endl;
	_subtask._state = TaskBuilderState::ERROR;
      }
    }
    Subtask& _subtask;
  } satisfyPromise(subtask);
  bool bcompressed = _options._compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    Logger(LOG_LEVEL::DEBUG) << "compression " << (bcompressed ? "enabled" : "disabled") << std::endl;
  size_t uncomprSize = end - beg - HEADER_SIZE;
  if (bcompressed) {
    try {
      std::string_view compressedView = Compression::compress(beg + HEADER_SIZE, uncomprSize);
      // LZ4 may generate compressed larger than uncompressed.
      // In this case an uncompressed subtask is sent.
      if (compressedView.size() >= uncomprSize) {
	encodeHeader(beg, HEADERTYPE::SESSION, uncomprSize, uncomprSize, COMPRESSORS::NONE, _options._diagnostics);
	subtask._chars.assign(beg, end);
      }
      else {
	encodeHeader(beg, HEADERTYPE::SESSION, uncomprSize, compressedView.size(), _options._compressor, _options._diagnostics);
	std::copy(compressedView.data(), compressedView.data() + compressedView.size(), beg + HEADER_SIZE);
	subtask._chars.assign(beg, beg + HEADER_SIZE + compressedView.size());
      }
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
      return TaskBuilderState::ERROR;
    }
  }
  else {
    encodeHeader(beg, HEADERTYPE::SESSION, uncomprSize, uncomprSize, _options._compressor, _options._diagnostics);
    subtask._chars.assign(beg, end);
  }
  if (subtask._state != TaskBuilderState::ERROR)
    subtask._state = alldone ? TaskBuilderState::TASKDONE : TaskBuilderState::SUBTASKDONE;
  return subtask._state;
}
