/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"

#include "ClientOptions.h"
#include "FileLines.h"
#include "IOUtility.h"

TaskBuilder::TaskBuilder(CryptoWeakPtr crypto) :
  _crypto(crypto), _subtaskIndex(0) {
  _aggregate.reserve(ClientOptions::_bufferSize);
}

void TaskBuilder::run() {
  while (!_stopped) {
    _resume = false;
    try {
      // can be a new or the same source for tests.
      FileLines lines(ClientOptions::_sourceName, '\n', true);
      while (createSubtask(lines) == STATUS::SUBTASK_DONE);
      std::unique_lock lock(_mutex);
      _conditionResume.wait(lock, [this] { return _resume || _stopped; });
    }
    catch (const std::exception& e) {
      LogError << e.what() << '\n';
      _status.store(STATUS::ERROR);
      break;
    }
  }
}

void TaskBuilder::getTask(Subtasks& task) {
  std::unique_lock lock(_mutex);
  _conditionTask.wait(lock, [this, task] {
    if (_subtasks.empty())
      return false;
    if (!_subtasks.empty()) {
      STATUS status = _subtasks.back()._state;
      switch (status) {
      case STATUS::TASK_DONE:
      case STATUS::ERROR:
      case STATUS::STOPPED:
	return true;
      default:
	return false;
      }
    }
    return true;
  });
  task.swap(_subtasks);
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
  _aggregate.clear();
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
    ClientOptions::_diagnostics,
    _status,
    0 };
  std::lock_guard lock(_mutex);
  if (_stopped)
    return STATUS::STOPPED;
  utility::compressEncrypt(
    _buffer, ClientOptions::_encrypted, header, _crypto, _aggregate);
  if (_subtaskIndex >= _subtasks.size())
    _subtasks.emplace_back();
  Subtask& subtask = _subtasks[_subtaskIndex];
  subtask._data.resize(_aggregate.size());
  std::memcpy(subtask._data.data(), _aggregate.data(), _aggregate.size());
  subtask._state = alldone ? STATUS::TASK_DONE : STATUS::SUBTASK_DONE;
  subtask._header.swap(header);
  switch (subtask._state) {
  case STATUS::TASK_DONE:
  case STATUS::ERROR:
    _subtasks.resize(_subtaskIndex + 1);
    _conditionTask.notify_one();
    break;
  case STATUS::SUBTASK_DONE:
    ++_subtaskIndex;
    break;
  default:
    break;
  }
  return subtask._state;
}

void TaskBuilder::stop() {
  std::lock_guard lock(_mutex);
  _stopped = true;
  _conditionResume.notify_one();
}

void TaskBuilder::resume() {
  std::lock_guard lock(_mutex);
  if (_stopped)
    return;
  _subtaskIndex = 0;
  _resume = true;
  _conditionResume.notify_one();
}
