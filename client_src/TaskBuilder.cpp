/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"

#include "ClientOptions.h"
#include "FileLines.h"
#include "IOUtility.h"
#include "Logger.h"

TaskBuilder::TaskBuilder(CryptoWeakPtr crypto) :
  _crypto(crypto),
  _subtasks(1) {
  _aggregate.reserve(ClientOptions::_bufferSize);
}

TaskBuilder::~TaskBuilder() {
  Trace << '\n';
}

void TaskBuilder::run() {
  while (!_stopped) {
    _resume = false;
    try {
      // can be a new or the same source for tests.
      FileLines lines(ClientOptions::_sourceName, '\n', true);
      while (createSubtask(lines) == STATUS::SUBTASK_DONE);
      std::unique_lock lock(_mutex);
      _condition.wait(lock, [this] { return _resume || _stopped; });
    }
    catch (const std::exception& e) {
      LogError << e.what() << '\n';
      _status.store(STATUS::ERROR);
      _condition.notify_one();
      break;
    }
  }
}

std::pair<STATUS, Subtasks&> TaskBuilder::getTask() {
  static Subtasks emptyTask;
  STATUS state = STATUS::NONE;
  do {
    Subtask& subtask = getSubtask();
    state = subtask._state;
    switch (state) {
    case STATUS::ERROR:
      LogError << "TaskBuilder failed." << '\n';
      return { STATUS::ERROR, emptyTask };
    case STATUS::SUBTASK_DONE:
    case STATUS::TASK_DONE:
      break;
    default:
      return { STATUS::ERROR, emptyTask };
    }
  } while (state == STATUS::SUBTASK_DONE);
  return { STATUS::NONE, _subtasks };
}

Subtask& TaskBuilder::getSubtask() {
  static Subtask emptySubtask;
  std::unique_lock lock(_mutex);
  std::optional<std::reference_wrapper<Subtask>> subtaskRef;
  _condition.wait(lock, [&] () {
    if (_subtaskIndexConsumed < _subtasks.size()) {
      if (_status == STATUS::ERROR)
	return true;
      subtaskRef = _subtasks[_subtaskIndexConsumed];
      STATUS state = subtaskRef->get()._state;
      if (state == STATUS::SUBTASK_DONE || state == STATUS::TASK_DONE)
	return true;
    }
    return false;
  });
  if (_stopped)
    return emptySubtask;
  _subtaskIndexConsumed.fetch_add(1);
  return subtaskRef->get();
}

void TaskBuilder::copyRequestWithId(std::string_view line, long index) {
  _aggregate.push_back('[');
  ioutility::toChars(index, _aggregate);
  _aggregate.push_back(']');
  _aggregate.append(line);
}

// Read requests from the source, generate id for each.
// The source can be a file or a message received from
// another device.
// Aggregate requests for sending many in one shot to
// reduce the number of system calls. The size of the
// aggregate depends on the buffer size.

STATUS TaskBuilder::createSubtask(Lines& lines) {
  // LogAlways << "\t### _aggregate.capacity()=" << _aggregate.capacity() << '\n';
  _aggregate.erase(_aggregate.cbegin(), _aggregate.cend());
  // lower bound estimate considering added id
  std::size_t maxSubtaskSize = ClientOptions::_bufferSize - HEADER_SIZE;
  std::string_view line;
  while (lines.getLine(line)) {
    copyRequestWithId(line, lines._index);
    bool alldone = lines._last;
    if (_aggregate.size() >= maxSubtaskSize || alldone)
      return compressEncryptSubtask(alldone);
  }
  return STATUS::NONE;
}

// Compress and encypt requests if options require.
// Generate header for every aggregated group of requests.
STATUS TaskBuilder::compressEncryptSubtask(bool alldone) {
  HEADER header{
    HEADERTYPE::SESSION,
    _aggregate.size(),
    ClientOptions::_compressor,
    ClientOptions::_diagnostics,
    _status,
    0 };
  utility::compressEncrypt(
    _buffer, ClientOptions::_encrypted, header, _crypto, _aggregate);
  std::lock_guard lock(_mutex);
  if (_stopped)
    return STATUS::STOPPED;
  std::optional<std::reference_wrapper<Subtask>> subtaskRef;
  unsigned index = _subtaskIndexProduced.fetch_add(1);
  if (index < _subtasks.size())
    subtaskRef = _subtasks[index];
  else
    subtaskRef = _subtasks.emplace_back();
  Subtask& subtask = subtaskRef->get();
  subtask._data.resize(_aggregate.size());
  std::memcpy(subtask._data.data(), _aggregate.data(), _aggregate.size());
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
  if (_stopped)
    return;
  _subtaskIndexConsumed = 0;
  _subtaskIndexProduced = 0;
  _resume = true;
  _condition.notify_one();
}
