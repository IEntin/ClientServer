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
  _options(options), _threadPool(3) {
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
    auto taskBuilder = std::make_shared<TaskBuilder>(_options);
    _threadPool.push(taskBuilder);
    do {
      Chronometer chronometer(_options._timing, __FILE__, __LINE__, __func__, _options._instrStream);
      auto savedBuild = std::move(taskBuilder);
      if (_options._runLoop) {
	// start construction of the next task in the background
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

bool Client::printReply(const std::vector<char>& buffer, const HEADER& header) {
  auto [headerType, uncomprSize, comprSize, compressor, diagnostics, problem] = header;
  bool bcompressed = compressor == COMPRESSORS::LZ4;
  std::string_view received(buffer.data(), comprSize);
  std::ostream* pstream = _options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  static auto& printOnce[[maybe_unused]] =
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << (bcompressed ? " received compressed." : " received not compressed.") << std::endl;
  if (bcompressed) {
    std::string_view dstView = Compression::uncompress(received, uncomprSize);
    stream << dstView << std::flush;
  }
  else
    stream << received << std::flush;
  switch (problem) {
  case PROBLEMS::NONE:
    break;
  case PROBLEMS::BAD_HEADER:
    CERR << "PROBLEMS::BAD_HEADER\n";
    break;
  case PROBLEMS::FIFO_PROBLEM:
    CERR << "PROBLEMS::FIFO_PROBLEM\n";
    break;
  case PROBLEMS::TCP_PROBLEM:
    CERR << "PROBLEMS::TCP_PROBLEM\n";
    break;
  case PROBLEMS::TCP_TIMEOUT:
    CERR << "\tserver timeout! Increase \"TcpTimeout\" in ServerOptions.json\n";
    break;
  case PROBLEMS::MAX_TOTAL_SESSIONS:
    CLOG << "PROBLEMS::MAX_TOTAL_SESSIONS" << std::endl;
    break;
  case PROBLEMS::MAX_NUMBER_RUNNABLES:
    CLOG << "PROBLEMS::MAX_NUMBER_RUNNABLES" << std::endl;
    break;
  default:
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":unexpected problem\n";
    break;
  }
  return true;
}
