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
#include <csignal>

extern volatile std::sig_atomic_t stopSignal;

Client::Client(const ClientOptions& options) : 
  _options(options) {
  MemoryPool::setExpectedSize(options._bufferSize);
}

Client::~Client() {
  _threadPool.stop();
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

// Allows to read and process the source in parts with sizes
// determined by the buffer size. This reduces memory footprint.
// For maximum speed the buffer should be large to read the
// content in one shot.

bool Client::processTask(TaskBuilderPtr taskBuilder) {
  std::vector<char> task;
  while (true) {
    TaskBuilderState state = taskBuilder->getTask(task);
    switch (state) {
    case TaskBuilderState::ERROR:
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":TaskBuilder failed.\n";
      return false;
    case TaskBuilderState::SUBTASKDONE:
    case TaskBuilderState::TASKDONE:
      if (!(send(task) && receive()))
	return false;
      if (state == TaskBuilderState::TASKDONE)
	return true;
      break;
    default:
      break;
    }
  }
  return true;
}

bool Client::run() {
  try {
    int numberTasks = 0;
    TaskBuilderPtr taskBuilder = std::make_shared<TaskBuilder>(_options);
    _threadPool.push(taskBuilder);
    do {
      Chronometer chronometer(_options._timing, __FILE__, __LINE__, __func__, _options._instrStream);
      TaskBuilderPtr savedBuild = taskBuilder;
      // start construction of the next task in the background
      if (_options._runLoop) {
	taskBuilder = std::make_shared<TaskBuilder>(_options);
	_threadPool.push(taskBuilder);
      }
      if (!processTask(savedBuild))
	return false;
      if (_options._maxNumberTasks > 0 && ++numberTasks == _options._maxNumberTasks)
	break;
    } while (_options._runLoop && !stopSignal);
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
  }
  return true;
}

bool Client::printReply(const std::vector<char>& buffer, size_t uncomprSize, size_t comprSize, bool bcompressed) {
  std::string_view received(buffer.data(), comprSize);
  std::ostream* pstream = _options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  if (bcompressed) {
    static auto& printOnce[[maybe_unused]] =
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " received compressed." << std::endl;
    std::string_view dstView = Compression::uncompress(received, uncomprSize);
    stream << dstView << std::flush;
  }
  else {
    static auto& printOnce[[maybe_unused]] =
      CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " received not compressed." << std::endl;
    stream << received << std::flush;
  }
  return true;
}
