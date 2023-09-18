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

void TaskBuilder::copyRequestWithId(std::string& aggregate, std::string_view line) {
  aggregate.push_back('[');
  utility::toChars(_requestIndex++, aggregate);
  aggregate.push_back(']');
  aggregate.insert(aggregate.cend(), line.cbegin(), line.cend());
  aggregate.push_back('\n');
}

// Read requests from the source, generate id for each.
// The source can be a file or a message received from
// another device.
// Aggregate requests for sending many in one shot to
// reduce the number of system calls. The size of the
// aggregate depends on the buffer size.

STATUS TaskBuilder::createSubtask() {
  static thread_local std::string aggregate;
  aggregate.resize(0);
  //LogAlways << "\t### " << aggregate.capacity() << '\n';
  size_t maxSubtaskSize = ClientOptions::_bufferSize * 0.9;
  thread_local static std::string line;
  line.resize(0);
  while (std::getline(_input, line)) {
    copyRequestWithId(aggregate, line);
    bool alldone = _input.peek() == std::istream::traits_type::eof();
    if (aggregate.size() >= maxSubtaskSize || alldone) {
      aggregate.pop_back();
      return compressEncryptSubtask(aggregate, alldone);
    }
  }
  return STATUS::NONE;
}

// Encrypt and compress requests if options require.
// Generate header for every aggregated group of requests.

STATUS TaskBuilder::compressEncryptSubtask(std::string_view data, bool alldone) {
  HEADER header{ HEADERTYPE::SESSION, 0, 0, ClientOptions::_compressor, ClientOptions::_encrypted, ClientOptions::_diagnostics, _status };
  std::string_view output = payloadtransform::compressEncrypt(data, header);
  std::scoped_lock lock(_mutex);
  _subtasks.emplace_back();
  Subtask& subtask = _subtasks.back();
  subtask._header = std::move(header);
  subtask._body = std::move(output);
  subtask._state = alldone ? STATUS::TASK_DONE : STATUS::SUBTASK_DONE;
  _condition.notify_one();
  return subtask._state;
}
