/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"
#include "ClientOptions.h"
#include "Header.h"
#include "PayloadTransform.h"
#include "Utility.h"

TaskBuilder::TaskBuilder() {
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
  std::unique_lock lock(_mutex);
  _condition.wait(lock, [this] { return !_subtasks.empty(); });
  auto& subtask = _subtasks.front();
  STATUS state = subtask._state;
  task._state = state;
  switch (state) {
  case STATUS::SUBTASK_DONE:
  case STATUS::TASK_DONE:
    subtask._header.swap(task._header);
    subtask._body.swap(task._body);
    break;
  default:
    break;
  }
  _subtasks.pop_front();
  return task._state;
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
  std::string aggregate;
  size_t aggregateSize = 0;
  // rough estimate for subtask size to minimize reallocation.
  size_t maxSubtaskSize = ClientOptions::_bufferSize * 0.9;
  thread_local static std::string line;
  while (std::getline(_input, line)) {
    aggregate.resize(aggregateSize + _nextIdSz + line.size() + 1);
    int copied = copyRequestWithId(aggregate.data() + aggregateSize, line);
    aggregateSize += copied;
    bool alldone = _input.peek() == std::istream::traits_type::eof();
    if (aggregateSize >= maxSubtaskSize || alldone) {
      // remove last eol
      aggregate.pop_back();
      return compressEncryptSubtask(aggregate, alldone);
    }
    else
      continue;
  }
  return STATUS::NONE;
}

// Encrypt and compress requests if options require.
// Generate header for every aggregated group of requests.

STATUS TaskBuilder::compressEncryptSubtask(std::string& data, bool alldone) {
  HEADER header{ HEADERTYPE::SESSION, 0, 0, ClientOptions::_compressor, ClientOptions::_encrypted, ClientOptions::_diagnostics, _status };
  payloadtransform::compressEncrypt(data, header);
  std::scoped_lock lock(_mutex);
  _subtasks.emplace_back();
  Subtask& subtask = _subtasks.back();
  subtask._header.swap(header);
  subtask._body.swap(data);
  subtask._state = alldone ? STATUS::TASK_DONE : STATUS::SUBTASK_DONE;
  _condition.notify_one();
  return subtask._state;
}
