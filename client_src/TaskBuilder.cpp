/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"
#include "ClientOptions.h"
#include "Header.h"
#include "PayloadTransform.h"
#include "Utility.h"
#include <cassert>

Subtask TaskBuilder::_emptySubtask;

TaskBuilder::TaskBuilder() {
  _aggregate.reserve(ClientOptions::_bufferSize);
  try {
    _input.open(ClientOptions::_sourceName, std::ios::binary);
    _input.exceptions(std::ifstream::failbit); // may throw
  }
  catch (const std::ios_base::failure& fail) {
    std::cout << fail.what() << '\n';
  }
}

TaskBuilder::~TaskBuilder() {
  Trace << '\n';
}

void TaskBuilder::run() {
  while (!_stopped) {
    _resume = false;
    while (createSubtask() == STATUS::SUBTASK_DONE) {}
    std::unique_lock lock(_mutex);
   _condition.wait(lock, [this] { return _resume || _stopped; });
  }
}

Subtask& TaskBuilder::getSubtask() {
  std::unique_lock lock(_mutex);
  _condition.wait(lock, [this] {
    return _subtaskIndexOut < _subtasks.size() || _stopped;
  });
  if (_stopped)
    return _emptySubtask;
  assert(_subtaskIndexOut < _subtasks.size());
  auto& subtask = _subtasks[_subtaskIndexOut];
  _subtaskIndexOut.fetch_add(1);
  return subtask;
}

void TaskBuilder::copyRequestWithId(std::string_view line) {
  _aggregate.push_back('[');
  unsigned index = _requestIndex.fetch_add(1);
  utility::toChars(index, _aggregate);
  _aggregate.push_back(']');
  _aggregate.insert(_aggregate.cend(), line.cbegin(), line.cend());
  _aggregate.push_back('\n');
}

// Read requests from the source, generate id for each.
// The source can be a file or a message received from
// another device.
// Aggregate requests for sending many in one shot to
// reduce the number of system calls. The size of the
// aggregate depends on the buffer size.

STATUS TaskBuilder::createSubtask() {
  // LogAlways << "\t### _aggregate.capacity()=" << _aggregate.capacity() << '\n';
  _aggregate.resize(0);
  // estimate of max size considering additional id
  size_t maxSubtaskSize = ClientOptions::_bufferSize * 0.9;
  while (std::getline(_input, _line)) {
    copyRequestWithId(_line);
    bool alldone = _input.peek() == std::istream::traits_type::eof();
    if (_aggregate.size() >= maxSubtaskSize || alldone) {
      _aggregate.pop_back();
      return compressEncryptSubtask(alldone);
    }
  }
  return STATUS::NONE;
}

// Encrypt and compress requests if options require.
// Generate header for every aggregated group of requests.

STATUS TaskBuilder::compressEncryptSubtask(bool alldone) {
  HEADER header{ HEADERTYPE::SESSION, 0, 0, ClientOptions::_compressor, ClientOptions::_encrypted, ClientOptions::_diagnostics, _status };
  std::string_view output = payloadtransform::compressEncrypt(_aggregate, header);
  std::scoped_lock lock(_mutex);
  if (_stopped)
    return STATUS::NONE;
  Subtask& subtask = _subtasks.emplace_back();
  subtask._header = std::move(header);
  subtask._body.assign(output.cbegin(), output.cend());
  subtask._state = alldone ? STATUS::TASK_DONE : STATUS::SUBTASK_DONE;
  _condition.notify_one();
  return subtask._state;
}

void TaskBuilder::stop() {
  std::scoped_lock lock(_mutex);
  _stopped = true;
  _condition.notify_one();
}

void TaskBuilder::resume() {
  std::scoped_lock lock(_mutex);
  _input.clear();
  _input.seekg(0);
  _subtasks.resize(0);
  _requestIndex = 0;
  _subtaskIndexOut = 0;
 _resume = true;
  _condition.notify_one();
}
