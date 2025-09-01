/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"

#include "ClientOptions.h"
#include "FileLines.h"
#include "IOUtility.h"
#include "Options.h"
#include "Utility.h"

TaskBuilder::TaskBuilder(CryptoVariant crypto) :
  _subtaskIndex(0),
  _crypto(crypto) {
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

std::pair<std::size_t, STATUS> TaskBuilder::getTask(Subtasks& task) {
  std::unique_lock lock(_mutex);
  _conditionTask.wait(lock, [this] {
    switch (_status) {
      case STATUS::TASK_DONE:
      case STATUS::ERROR:
      case STATUS::STOPPED:
	return true;
    default:
      return false;
    }
  });
  task.swap(_subtasks);
  return { _subtaskIndex + 1, _status };
}

void TaskBuilder::copyRequestWithId(std::string_view line, long index) {
  _aggregate += '[';
  ioutility::toChars(index, _aggregate);
  _aggregate += ']';
  _aggregate += line;
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
  std::string_view dataView =
    cryptodefinitions::compressEncrypt(_crypto, _buffer, header, _aggregate, ClientOptions::_doEncrypt);
  if (_subtaskIndex >= _subtasks.size())
    _subtasks.emplace_back();
  Subtask& subtask = _subtasks[_subtaskIndex];
  subtask._data = dataView;
  _status = alldone ? STATUS::TASK_DONE : STATUS::SUBTASK_DONE;
  subtask._header.swap(header);
  switch (_status) {
  case STATUS::TASK_DONE:
  case STATUS::ERROR:
    _conditionTask.notify_one();
    break;
  case STATUS::SUBTASK_DONE:
    ++_subtaskIndex;
    break;
  default:
    break;
  }
  return _status;
}

void TaskBuilder::stop() {
  std::lock_guard lock(_mutex);
  _stopped = true;
  _status = STATUS::STOPPED;
  _conditionResume.notify_one();
}

void TaskBuilder::resume() {
  std::lock_guard lock(_mutex);
  if (_stopped)
    return;
  _subtaskIndex = 0;
  _status = STATUS::NONE;
  _resume = true;
  _conditionResume.notify_one();
}
