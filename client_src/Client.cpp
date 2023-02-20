/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Client.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Metrics.h"
#include "TaskBuilder.h"
#include "TcpClientHeartbeat.h"
#include "Utility.h"

std::atomic_flag Client::_stopFlag;

Client::Client(const ClientOptions& options) : _options(options) {}

Client::~Client() {
  Trace << std::endl;
}

// Allows to read and process the source in parts with sizes
// determined by the buffer size. This reduces memory footprint.
// For maximum speed the buffer should be large to read the
// content in one shot.

bool Client::processTask(TaskBuilderPtr taskBuilder) {
  // static here is safe and saves on memory allocations
  static Subtask task;
  while (true) {
    TaskBuilderState state = taskBuilder->getSubtask(task);
    switch (state) {
    case TaskBuilderState::ERROR:
      LogError << "TaskBuilder failed." << std::endl;
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
  struct Destroy {
    Destroy(Client* client) : _client(client) {}
    ~Destroy() {
      _client->stop();
    }
    Client* _client = nullptr;
  } destroy(this);
  try {
    int numberTasks = 0;
    _taskBuilder = std::make_shared<TaskBuilder>(_options, _threadPoolClient);
    _threadPoolClient.push(_taskBuilder);
    do {
      Chronometer chronometer(_options._timing, __FILE__, __LINE__, __func__, _options._instrStream);
      auto savedBuild = std::move(_taskBuilder);
      if (_options._runLoop) {
	// start construction of the next task in the background
	_taskBuilder = std::make_shared<TaskBuilder>(_options, _threadPoolClient);
	_threadPoolClient.push(_taskBuilder);
      }
      if (!processTask(savedBuild))
	return false;
      if (_options._maxNumberTasks > 0 && ++numberTasks == _options._maxNumberTasks)
	break;
    } while (_options._runLoop);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
  return true;
}

void Client::stop() {
  Metrics::save();
  destroySession();
  if (_heartbeat)
    _heartbeat->stop();
  if (_taskBuilder)
    _taskBuilder->stop();
  _threadPoolClient.stop();
}

bool Client::printReply(const std::vector<char>& buffer, const HEADER& header) {
  if (_heartbeat) {
    STATUS status = _heartbeat->getStatus();
    if (utility::displayStatus(status))
      return false;
  }
  auto [headerType, uncomprSize, comprSize, compressor, diagnostics, status] = header;
  if (utility::displayStatus(status))
    return false;
  bool bcompressed = isCompressed(header);
  std::ostream* pstream = _options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  static auto& printOnce[[maybe_unused]] =
    Trace << (bcompressed ? " received compressed." : " received not compressed.") << std::endl;
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

void Client::start() {
  try {
    if (_options._enableHeartbeat) {
      _heartbeat = std::make_shared<tcp::TcpClientHeartbeat>(_options, _threadPoolClient);
      _heartbeat->start();
    }
  }
  catch (const std::exception& e) {
    Warn << e.what() << std::endl;
  }
}
