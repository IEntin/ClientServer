/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Client.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "TaskBuilder.h"
#include <cassert>
#include <csignal>
#include <cstring>
#include <filesystem>

Client::Client(const ClientOptions& options) : _options(options), _threadPool(options._numberBuilderThreads) {
  _memoryPool.setInitialSize(options._bufferSize);
}

Client::~Client() {
  _threadPool.stop();
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool Client::processSubtask(const std::vector<char>& subtask) {
  if (!send(subtask))
    return false;
  if (!receive())
    return false;
  return true;
}

// Allows to read and process the source in parts with sizes
// determined by the buffer size. This reduces memory footprint.
// For maximum speed the buffer is large and the content is
// read in one shot.

bool Client::processTask(TaskBuilderPtr&& taskBuilder) {
  std::vector<char> task;
  TaskBuilderState state = TaskBuilderState::NONE;
  // process the first subtask (it can be the only one):
  bool success = taskBuilder->getTask(task);
  if (!success) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":TaskBuilder failed" << std::endl;
    return false;
  }
  if (!processSubtask(task))
    return false;
  state = taskBuilder->getState();
  if (state == TaskBuilderState::TASKDONE)
    return true;
  // process all remaining subtasks except the last:
  while (state == TaskBuilderState::SUBTASKDONE) {
    // Blocks until task construction in another thread is finished
    bool success = taskBuilder->getTask(task);
    if (!success) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":TaskBuilder failed" << std::endl;
      return false;
    }
    state = taskBuilder->getState();
    if (!processSubtask(task))
      return false;
  }
  // process the last subtask, state is TaskBuilderState::TASKDONE
  success = taskBuilder->getTask(task);
  if (!success) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":TaskBuilder failed" << std::endl;
    return false;
  }
  if (!processSubtask(task))
    return false;
  return true;
}

bool Client::run() {
  try {
    std::error_code ec;
    _sourceSize = std::filesystem::file_size(_options._sourceName, ec);
    if (ec) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ':' << ec.message() << ':' << _options._sourceName << '\n';
      return false;
    }
    int numberTasks = 0;
    TaskBuilderPtr taskBuilder = std::make_shared<TaskBuilder>(_options, _memoryPool, _sourceSize);
    _threadPool.push(taskBuilder);
    do {
      Chronometer chronometer(_options._timing, __FILE__, __LINE__, __func__, _options._instrStream);
      TaskBuilderPtr savedBuild = std::move(taskBuilder);
      // start construction of the next task in the background
      if (_options._runLoop) {
	taskBuilder = std::make_shared<TaskBuilder>(_options, _memoryPool, _sourceSize);
	_threadPool.push(taskBuilder);
      }
      if (!processTask(std::move(savedBuild)))
	return false;
      if (_options._maxNumberTasks > 0 && ++numberTasks == _options._maxNumberTasks)
	break;
    } while (_options._runLoop);
  }
  catch (const std::exception& e) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	      << e.what() << ' ' << _options._sourceName << std::endl;
  }
  return true;
}

bool Client::printReply(const std::vector<char>& buffer, size_t uncomprSize, size_t comprSize, bool bcompressed) {
  std::string_view received(buffer.data(), comprSize);
  std::ostream* pstream = _options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
						       << " received compressed" << std::endl;
    std::string_view dstView = Compression::uncompress(received, uncomprSize, _memoryPool);
    if (dstView.empty()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
      return false;
    }
    stream << dstView; 
  }
  else {
    static auto& printOnce[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
						       << " received not compressed" << std::endl;
    stream << received;
  }
  return true;
}
