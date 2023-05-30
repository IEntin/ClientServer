/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "ThreadPoolBase.h"
#include "Utility.h"

TaskBuilder::TaskBuilder(const ClientOptions& options, ThreadPoolBase& threadPool) :
  _options(options),
  _subtasks(1),
  _threadPool(threadPool) {
  _input.open(_options._sourceName, std::ios::binary);
  if(!_input)
    throw std::ios::failure("Error opening file");
}

TaskBuilder::~TaskBuilder() {
  Trace << std::endl;
}

void TaskBuilder::run() {
  try {
    while (createSubtask() == TaskBuilderState::SUBTASKDONE) {}
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
  catch (...) {
    LogError << "exception caught." << std::endl;
  }
}

void TaskBuilder::stop() {
  _stopped = true;
  Subtask subtask;
  getSubtask(subtask);
}

TaskBuilderState TaskBuilder::getSubtask(Subtask& task) {
  TaskBuilderState state = TaskBuilderState::NONE;
  if (_stopped)
    return TaskBuilderState::STOPPED;
  try {
    auto& subtask = _subtasks.front();
    subtask._state.wait(TaskBuilderState::NONE);
    state = subtask._state;
    switch (state) {
    case TaskBuilderState::SUBTASKDONE:
    case TaskBuilderState::TASKDONE:
      subtask._header.swap(task._header);
      subtask._body.swap(task._body);
      break;
    default:
      break;
    }
    std::scoped_lock lock(_mutex);
    _subtasks.pop_front();
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
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
// Aggregate requests for sending many in one shot to
// reduce the number of system calls. The size of the
// aggregate depends on the configured buffer size.

TaskBuilderState TaskBuilder::createSubtask() {
  thread_local static std::vector<char> aggregate(_options._bufferSize);
  size_t aggregateSize = 0;
  // rough estimate for subtask size to minimize reallocation.
  size_t maxSubtaskSize = _options._bufferSize * 0.9;
  thread_local static std::string line;
  auto& subtask = _subtasks.back();
  {
    std::scoped_lock lock(_mutex);
    _subtasks.emplace_back();
  }
  try {
    while (std::getline(_input, line)) {
      aggregate.reserve(aggregateSize + _nextIdSz + line.size() + 1);
      int copied = copyRequestWithId(aggregate.data() + aggregateSize, line);
      aggregateSize += copied;
      bool alldone = _input.peek() == std::istream::traits_type::eof();
      if (aggregateSize >= maxSubtaskSize || alldone)
	return compressSubtask(subtask, aggregate, aggregateSize, alldone);
      else
	continue;
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    subtask._state = TaskBuilderState::ERROR;
    subtask._state.notify_one();
    return TaskBuilderState::ERROR;
  }
  return TaskBuilderState::NONE;
}

// Compress requests if options require.
// Generate header for every aggregated group of requests.

TaskBuilderState TaskBuilder::compressSubtask(Subtask& subtask,
					      const std::vector<char>& aggregate,
					      size_t aggregateSize,
					      bool alldone) {
  bool bcompressed = _options._compressor == COMPRESSORS::LZ4;
  static auto& printOnce[[maybe_unused]] =
    Logger(LOG_LEVEL::DEBUG) << "compression " << (bcompressed ? "enabled" : "disabled") << std::endl;
  size_t uncomprSize = aggregateSize;
  HEADER header;
  std::vector<char> body;
  if (bcompressed) {
    try {
      std::string_view compressedView = Compression::compress(aggregate.data(), uncomprSize);
      // LZ4 may generate compressed larger than uncompressed.
      // In this case an uncompressed subtask is sent.
      if (compressedView.size() >= uncomprSize) {
	header = { HEADERTYPE::SESSION,
	  uncomprSize,
	  uncomprSize,
	  COMPRESSORS::NONE,
	  _options._diagnostics,
	  STATUS::NONE };
	body.assign(aggregate.data(), aggregate.data() + aggregateSize);
      }
      else {
	header = { HEADERTYPE::SESSION,
	  uncomprSize,
	  compressedView.size(),
	  _options._compressor,
	  _options._diagnostics,
	  STATUS::NONE };
	body.assign(compressedView.cbegin(), compressedView.cend());
      }
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
      return TaskBuilderState::ERROR;
    }
  }
  else {
    header = { HEADERTYPE::SESSION,
      uncomprSize,
      uncomprSize,
      _options._compressor,
      _options._diagnostics,
      STATUS::NONE };
    body.assign(aggregate.data(), aggregate.data() + aggregateSize);
  }
  std::scoped_lock lock(_mutex);
  subtask._header.swap(header);
  subtask._body.swap(body);
  subtask._state = alldone ? TaskBuilderState::TASKDONE : TaskBuilderState::SUBTASKDONE;
  subtask._state.notify_one();
  return subtask._state;
}
