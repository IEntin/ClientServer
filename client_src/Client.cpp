/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Client.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "TaskBuilder.h"
#include "Utility.h"

std::atomic_flag Client::_stopFlag = ATOMIC_FLAG_INIT;

Client::Client(const ClientOptions& options) : _options(options) {}

Client::~Client() {
  _threadPoolTaskBuilder.stop();
  _threadPoolTcpHeartbeat.stop();
  _stopFlag.clear();
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
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":TaskBuilder failed." << std::endl;
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
    _threadPoolTaskBuilder.push(taskBuilder);
    do {
      Chronometer chronometer(_options._timing, __FILE__, __LINE__, __func__, _options._instrStream);
      auto savedBuild = std::move(taskBuilder);
      if (_options._runLoop) {
	// start construction of the next task in the background
	taskBuilder = std::make_shared<TaskBuilder>(_options);
	_threadPoolTaskBuilder.push(taskBuilder);
      }
      if (!processTask(savedBuild))
	return false;
      if (_options._maxNumberTasks > 0 && ++numberTasks == _options._maxNumberTasks)
	break;
    } while (_options._runLoop && !_stopFlag.test());
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << std::endl;
  }
  return true;
}

bool Client::printReply(const std::vector<char>& buffer, const HEADER& header) {
  auto [headerType, uncomprSize, comprSize, compressor, diagnostics, status] = header;
  if (utility::displayStatus(status))
    return false;
  bool bcompressed = compressor == COMPRESSORS::LZ4;
  std::ostream* pstream = _options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  static auto& printOnce[[maybe_unused]] =
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << (bcompressed ? " received compressed." : " received not compressed.") << std::endl;
  if (bcompressed) {
    std::string_view uncompressedView = Compression::uncompress(buffer.data(), comprSize, uncomprSize);
    if (uncompressedView.empty()) {
      utility::displayStatus(STATUS::DECOMPRESSION_PROBLEM);
      return false;
    }
    else
      stream << uncompressedView << std::flush;
  }
  else
    stream << std::string_view(buffer.data(), comprSize) << std::flush;
  return true;
}

void Client::setStopFlag() {
  _stopFlag.test_and_set();
}

bool Client::stopped() {
  return _stopFlag.test();
}
