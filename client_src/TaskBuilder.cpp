/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "IOUtility.h"
#include "TaskBuilder.h"

TaskBuilder::TaskBuilder(const std::tuple<CryptoWeakSodiumPtr, CryptoWeakPlPlPtr>& encryptors) :
  _encryptors(encryptors),
  _subtaskIndex(0),
  _linesR() {
  _batch.reserve(ClientOptions::_bufferSize);
  _subtasks.reserve(30);
  createBatches();
}

void TaskBuilder::run() {
  const std::size_t batchesSize = _batches.size();
  std::size_t batchIndex = 0;
  while (!_stopped) {
    _resume = false;
    try {
      // can be a new or the same source for tests.
      _linesR.resetIndices();
      while (batchIndex < batchesSize && processSubtask(batchIndex) == STATUS::SUBTASK_DONE);
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
// Read requests from the source, generate id for each.
// The source can be a file or a message received from
// another device.
// Aggregate requests for sending many in one shot to
// reduce the number of system calls. The size of the
// aggregate depends on the buffer size.

STATUS TaskBuilder::processSubtask(std::size_t batchIndex) {
  const std::size_t batchesSize = _batches.size();
  bool alldone = batchIndex == batchesSize - 1;
  _batch = _batches[batchIndex];
  return compressEncryptSubtask(alldone);
}

// Compress and encypt requests if options require.
// Generate header for every aggregated group of requests.
STATUS TaskBuilder::compressEncryptSubtask(bool alldone) {
  HEADER header{
    HEADERTYPE::SESSION,
    0,
    0,
    ClientOptions::_compressor,
    ClientOptions::_diagnostics,
    _status,
    0,
    0 };
  std::lock_guard lock(_mutex);
  if (_stopped)
    return STATUS::STOPPED;
  static thread_local std::string encrypted;
  encrypted.clear();
  encrypted = Options::_doubleEncryption ?
    compressDoubleEncrypt(_encryptors, _buffer, header, _batch, ClientOptions::_doEncrypt) :
    compressSingleEncrypt(_encryptors, Options::_singleEncryptor, _buffer, header, _batch, ClientOptions::_doEncrypt);
  std::string_view dataView = encrypted;
  std::get<std::to_underlying(HEADER_INDEX::FIELD1SIZEINDEX)>(header) = dataView.size();
  if (_subtaskIndex >= _subtasks.size())
    _subtasks.emplace_back();
  Subtask& subtask = _subtasks[_subtaskIndex];
  subtask._data.assign(dataView.data(), dataView.size());
  _status = alldone ? STATUS::TASK_DONE : STATUS::SUBTASK_DONE;
  subtask._header = header;
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

bool TaskBuilder::createBatches() {
  _linesR.resetIndices();
  const std::size_t maxSubtaskSize = ClientOptions::_bufferSize - HEADER_SIZE;
  const std::size_t vectorSize = _linesR._linesWithId.size();
  _batches.reserve(5);
  std::string batch;
  bool allDone = false;
  std::string_view lineWithId;
  while (_linesR.getLineWithId(lineWithId)) {
    batch.append(lineWithId);
    allDone = _linesR._linesWithIdIndex == vectorSize - 1;
    if (_batch.size() >= maxSubtaskSize || allDone) {
     _batches.push_back(batch);
      batch.clear();
    }
    if (!allDone)
      continue;
  }
  return true;
}
