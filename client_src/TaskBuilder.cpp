/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TaskBuilder.h"
#include "ClientOptions.h"
#include "CommonUtils.h"
#include "Crypto.h"
#include "Header.h"
#include "Utility.h"

TaskBuilder::TaskBuilder(const ClientOptions& options, CryptoKeys& cryptoKeys) :
  _options(options),
  _cryptoKeys(cryptoKeys),
  _subtasks(1) {
  _input.open(_options._sourceName, std::ios::binary);
  if(!_input)
    throw std::ios::failure("Error opening file");
}

TaskBuilder::~TaskBuilder() {
  Trace << std::endl;
}

void TaskBuilder::run() {
  try {
    while (createSubtask() == STATUS::SUBTASK_DONE) {}
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
  catch (...) {
    LogError << "exception caught." << std::endl;
  }
}

STATUS TaskBuilder::getSubtask(Subtask& task) {
  STATUS state = STATUS::NONE;
  try {
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
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return STATUS::ERROR;
  }
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
  thread_local static std::vector<char> aggregate;
  size_t aggregateSize = 0;
  // rough estimate for subtask size to minimize reallocation.
  size_t maxSubtaskSize = _options._bufferSize * 0.9;
  thread_local static std::string line;
  auto& subtask = _subtasks.back();
  {
    std::scoped_lock lock(_mutex);
    _subtasks.emplace_back();
  }
  try {
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
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    subtask._state = STATUS::ERROR;
    subtask._state.notify_one();
    return STATUS::ERROR;
  }
  return STATUS::NONE;
}

// Encrypt and compress requests if options require.
// Generate header for every aggregated group of requests.

STATUS TaskBuilder::encryptCompressSubtask(Subtask& subtask,
					   const std::vector<char>& data,
					   bool alldone) {
  HEADER header;
  static thread_local std::vector<char> body;
  body.clear();
  STATUS status =
    commonutils::encryptCompressData(_options, _cryptoKeys, data, header, body, _options._diagnostics);
  bool failed = false;
  switch (status) {
  case STATUS::ERROR:
  case STATUS::COMPRESSION_PROBLEM:
  case STATUS::ENCRYPTION_PROBLEM:
    failed = true;
    break;
  default:
    break;
  }
  if (failed)
    return status;
  std::scoped_lock lock(_mutex);
  subtask._header.swap(header);
  subtask._body.swap(body);
  subtask._state = alldone ? STATUS::TASK_DONE : STATUS::SUBTASK_DONE;
  subtask._state.notify_one();
  return subtask._state;
}
