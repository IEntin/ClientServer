/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Client.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "CommonUtils.h"
#include "Header.h"
#include "Metrics.h"
#include "TaskBuilder.h"
#include "TcpClientHeartbeat.h"
#include "Utility.h"
#include "WaitSignal.h"

std::atomic<ACTIONS> Client::_closeFlag = ACTIONS::NONE;

Client::Client(const ClientOptions& options) : _options(options) {}

Client::~Client() {
  stop();
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
    STATUS state = taskBuilder->getSubtask(task);
    switch (state) {
    case STATUS::ERROR:
      LogError << "TaskBuilder failed." << std::endl;
      return false;
    case STATUS::SUBTASK_DONE:
    case STATUS::TASK_DONE:
      if (!(send(task) && receive()))
	return false;
      if (state == STATUS::TASK_DONE)
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
  if (_heartbeat)
    _heartbeat->stop();
  if (_waitSignal)
    _waitSignal->stop();
  _threadPoolClient.stop();
}

bool Client::printReply(const HEADER& header, const std::vector<char>& buffer) {
  if (_heartbeat) {
    STATUS status = _heartbeat->getStatus();
    if (utility::displayStatus(status))
      return false;
  }
  std::ostream* pstream = _options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  std::string_view output =  commonutils::decompressDecrypt(header, buffer);
  if (output.empty()) {
    utility::displayStatus(STATUS::ERROR);
    return false;
  }
  stream << output << std::flush;
  return true;
}

void Client::start() {
  try {
    if (_options._heartbeatEnabled) {
      _heartbeat = std::make_shared<tcp::TcpClientHeartbeat>(_options, _threadPoolClient);
      _heartbeat->start();
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
}
