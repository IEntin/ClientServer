/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Client.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "MemoryPool.h"
#include "TaskBuilder.h"
#include "Utility.h"
#include <cassert>
#include <csignal>
#include <cstring>

Client::Client(const ClientOptions& options) : _options(options), _threadPool(options._numberBuilderThreads) {
  MemoryPool::setExpectedSize(options._bufferSize);
}

Client::~Client() {
  _threadPool.stop();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

// Allows to read and process the source in parts with sizes
// determined by the buffer size. This reduces memory footprint.
// For maximum speed the buffer should be large to read the
// content in one shot.

bool Client::processTask(TaskBuilderPtr&& taskBuilder) {
  std::vector<char> task;
  TaskBuilderState state = TaskBuilderState::NONE;
  do {
    state = taskBuilder->getTask(task);
    if (state == TaskBuilderState::ERROR) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	   << ":TaskBuilder failed.\n";
      return false;
    }
    bool subtaskDone = send(task) && receive();
    if (!subtaskDone) {
      std::exit(1);
      return false;
    }
  } while (state != TaskBuilderState::TASKDONE);
  return true;
}

bool Client::run() {
  try {
    int numberTasks = 0;
    _taskBuilder = std::make_shared<TaskBuilder>(_options);
    _threadPool.push(_taskBuilder);
    do {
      Chronometer chronometer(_options._timing, __FILE__, __LINE__, __func__, _options._instrStream);
      TaskBuilderPtr savedBuild = std::move(_taskBuilder);
      // start construction of the next task in the background
      if (_options._runLoop) {
	_taskBuilder = std::make_shared<TaskBuilder>(_options);
	_threadPool.push(_taskBuilder);
      }
      if (!processTask(std::move(savedBuild)))
	return false;
      if (_options._maxNumberTasks > 0 && ++numberTasks == _options._maxNumberTasks)
	break;
    } while (_options._runLoop);
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	 << e.what() << ' ' << _options._sourceName << '\n';
  }
  return true;
}

bool Client::printReply(const std::vector<char>& buffer, size_t uncomprSize, size_t comprSize, bool bcompressed) {
  std::string_view received(buffer.data(), comprSize);
  std::ostream* pstream = _options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] =
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	   << " received compressed.\n";
    std::string_view dstView = Compression::uncompress(received, uncomprSize);
    if (dstView.empty()) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	   << ":failed to uncompress payload.\n";
      return false;
    }
    stream << dstView; 
  }
  else {
    static auto& printOnce[[maybe_unused]] =
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	   << " received not compressed.\n";
    stream << received;
  }
  return true;
}
