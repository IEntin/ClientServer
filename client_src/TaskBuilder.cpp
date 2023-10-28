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

TaskBuilder::TaskBuilder() :
  _lines(ClientOptions::_sourceName, '\n', true) {
  _aggregate.reserve(ClientOptions::_bufferSize);
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
  std::reference_wrapper subtaskRef = _emptySubtask;
  _condition.wait(lock, [&] () mutable {
    if (_subtaskIndexConsumed < _subtasks.size()) {
      subtaskRef = _subtasks[_subtaskIndexConsumed];
      STATUS state = subtaskRef.get()._state;
      if (state == STATUS::SUBTASK_DONE || state == STATUS::TASK_DONE)
	return true;	
    }
    return false;
  });
  if (_stopped)
    return _emptySubtask;
  _subtaskIndexConsumed.fetch_add(1);
  return subtaskRef.get();
}

void TaskBuilder::copyRequestWithId(std::string_view line) {
  _aggregate.push_back('[');
  utility::toChars(_lines._index, _aggregate);
  _aggregate.push_back(']');
  _aggregate.append(line);
}

// Read requests from the source, generate id for each.
// The source can be a file or a message received from
// another device.
// Aggregate requests for sending many in one shot to
// reduce the number of system calls. The size of the
// aggregate depends on the buffer size.

STATUS TaskBuilder::createSubtask() {
  // LogAlways << "\t### _aggregate.capacity()=" << _aggregate.capacity() << '\n';
  _aggregate.erase(_aggregate.begin(), _aggregate.end());
  // estimate of max size considering additional id
  size_t maxSubtaskSize = ClientOptions::_bufferSize * 0.9;
  std::string_view line;
  while (_lines.getLine(line)) {
    copyRequestWithId(line);
    bool alldone = _lines._last;
    if (_aggregate.size() >= maxSubtaskSize || alldone)
      return compressEncryptSubtask(alldone);
  }
  return STATUS::NONE;
}

// Encrypt and compress requests if options require.
// Generate header for every aggregated group of requests.
STATUS TaskBuilder::compressEncryptSubtask(bool alldone) {
  HEADER header{ HEADERTYPE::SESSION, 0, 0, ClientOptions::_compressor, ClientOptions::_encrypted, ClientOptions::_diagnostics, _status };
  std::string_view output = payloadtransform::compressEncrypt(_aggregate, header);
  std::lock_guard lock(_mutex);
  if (_stopped)
    return STATUS::NONE;
  std::reference_wrapper<Subtask> subtaskRef = _emptySubtask;
  unsigned index = _subtaskIndexProduced.fetch_add(1);
  if (index >= _subtasks.size())
    subtaskRef = _subtasks.emplace_back();
  else
    subtaskRef = _subtasks[index];
  Subtask& subtask = subtaskRef.get();
  subtask._header = header;
  subtask._body.assign(output.data(), output.size());
  assert(static_cast<size_t>(extractPayloadSize(subtask._header)) == output.size());
  subtask._state = alldone ? STATUS::TASK_DONE : STATUS::SUBTASK_DONE;
  _condition.notify_one();
  return subtask._state;
}

void TaskBuilder::stop() {
  std::lock_guard lock(_mutex);
  _stopped = true;
  _condition.notify_one();
}

void TaskBuilder::resume() {
  std::lock_guard lock(_mutex);
  // in real cases each time another source is used
  _lines.reset(ClientOptions::_sourceName);
  for (auto& subtask : _subtasks)
    subtask._state = STATUS::NONE;
  _subtaskIndexConsumed = 0;
  _subtaskIndexProduced = 0;
  _resume = true;
  _condition.notify_one();
}
