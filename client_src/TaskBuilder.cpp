/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"

#include "ClientOptions.h"
#include "IOUtility.h"
#include "FileLines.h"
#include "Logger.h"

Subtask TaskBuilder::_emptySubtask;

Subtasks TaskBuilder::_emptyTask;

TaskBuilder::TaskBuilder(const CryptoPP::SecByteBlock& key) : _key(key), _subtasks(1) {
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
      while (createSubtask(lines) == STATUS::SUBTASK_DONE) {}
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

std::tuple<STATUS, Subtasks&> TaskBuilder::getResult() {
  STATUS state = STATUS::NONE;
  do {
    Subtask& subtask = getSubtask();
    state = subtask._state;
    switch (state) {
    case STATUS::ERROR:
      LogError << "TaskBuilder failed." << '\n';
      return { STATUS::ERROR, _emptyTask };
    case STATUS::SUBTASK_DONE:
    case STATUS::TASK_DONE:
      break;
    default:
      return { STATUS::ERROR, _emptyTask };
    }
  } while (state == STATUS::SUBTASK_DONE);
  return { STATUS::NONE, _subtasks };
}

Subtask& TaskBuilder::getSubtask() {
  std::unique_lock lock(_mutex);
  std::reference_wrapper subtaskRef = _emptySubtask;
  _condition.wait(lock, [&] () {
    if (_subtaskIndexConsumed < _subtasks.size()) {
      if (_status == STATUS::ERROR)
	return true;
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
  _aggregate.erase(0);
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
    0,
    _aggregate.size(),
    ClientOptions::_compressor,
    ClientOptions::_encrypted,
    ClientOptions::_diagnostics,
    _status,
    0 };
  std::string_view data = utility::compressEncrypt(header, _key, _aggregate);
  std::lock_guard lock(_mutex);
  if (_stopped)
    return STATUS::STOPPED;
  std::reference_wrapper<Subtask> subtaskRef = _subtasks[0];
  unsigned index = _subtaskIndexProduced.fetch_add(1);
  if (index < _subtasks.size())
    subtaskRef = _subtasks[index];
  else
    subtaskRef = _subtasks.emplace_back();
  Subtask& subtask = subtaskRef.get();
  subtask._body = data;
  subtask._header = header;
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
  for (auto& subtask : _subtasks)
    subtask._state = STATUS::NONE;
  _subtaskIndexConsumed = 0;
  _subtaskIndexProduced = 0;
  _resume = true;
  _condition.notify_one();
}
