/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"
#include "ClientOptions.h"
#include "PayloadTransform.h"
#include "Header.h"
#include "Utility.h"

TaskBuilder::TaskBuilder(const ClientOptions& options) :
  _options(options),
  _subtasks(1) {
  _input.open(ClientOptions::_sourceName, std::ios::binary);
  if(!_input)
    throw std::ios::failure("Error opening file");
}

TaskBuilder::~TaskBuilder() {
  Trace << '\n';
}

void TaskBuilder::run() {
  while (createSubtask() == STATUS::SUBTASK_DONE) {}
}

STATUS TaskBuilder::getSubtask(Subtask& task) {
  STATUS state = STATUS::NONE;
  auto& subtask = _subtasks.front();
  subtask._state.wait(STATUS::NONE);
  state = subtask._state;
  switch (state) {
  case STATUS::SUBTASK_DONE:
  case STATUS::TASK_DONE:
    subtask._header.swap(task._header);
    subtask._body.swap(task._body);
    break;
  default:
    break;
  }
  std::scoped_lock lock(_mutex);
  _subtasks.pop_front();
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
  std::copy(line.cbegin(), line.cend(), dst + offset);
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

STATUS TaskBuilder::createSubtask() {
  thread_local static std::string aggregate;
  size_t aggregateSize = 0;
  // rough estimate for subtask size to minimize reallocation.
  size_t maxSubtaskSize = _options._bufferSize * 0.9;
  thread_local static std::string line;
  auto& subtask = _subtasks.back();
  {
    std::scoped_lock lock(_mutex);
    _subtasks.emplace_back();
  }
  while (std::getline(_input, line)) {
    aggregate.resize(aggregateSize + _nextIdSz + line.size() + 1);
    int copied = copyRequestWithId(aggregate.data() + aggregateSize, line);
    aggregateSize += copied;
    bool alldone = _input.peek() == std::istream::traits_type::eof();
    if (aggregateSize >= maxSubtaskSize || alldone) {
      // remove last eol
      aggregate.pop_back();
      return encryptCompressSubtask(subtask, aggregate, alldone);
    }
    else
      continue;
  }
  return STATUS::NONE;
}

// Encrypt and compress requests if options require.
// Generate header for every aggregated group of requests.

STATUS TaskBuilder::encryptCompressSubtask(Subtask& subtask, std::string& data, bool alldone) {
  HEADER header;
  payloadtransform::compressEncrypt(_options, data, header, ClientOptions::_diagnostics);
  std::scoped_lock lock(_mutex);
  subtask._header.swap(header);
  subtask._body.swap(data);
  subtask._state = alldone ? STATUS::TASK_DONE : STATUS::SUBTASK_DONE;
  subtask._state.notify_one();
  return subtask._state;
}
