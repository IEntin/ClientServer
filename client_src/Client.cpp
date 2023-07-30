/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Client.h"
#include "Chronometer.h"
#include "ClientOptions.h"
#include "PayloadTransform.h"
#include "Metrics.h"
#include "TaskBuilder.h"
#include "TcpClientHeartbeat.h"
#include "Utility.h"

Client::Client(const ClientOptions& options) :
  _options(options) {}

Client::~Client() {
  stop();
  Trace << '\n';
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
      LogError << "TaskBuilder failed." << '\n';
      return false;
    case STATUS::SUBTASK_DONE:
    case STATUS::TASK_DONE:
      if (!(send(task) && receive()))
	return false;
      if (state == STATUS::TASK_DONE)
	return true;
      break;
    default:
      return false;
      break;
    }
  }
  return true;
}

bool Client::run() {
  int numberTasks = 0;
  auto taskBuilder = std::make_shared<TaskBuilder>(_options);
  _threadPoolClient.push(taskBuilder);
  do {
    Chronometer chronometer(_options._timing, __FILE__, __LINE__, __func__, _options._instrStream);
    auto savedBuild = std::move(taskBuilder);
    if (_options._runLoop) {
      // start construction of the next task in the background
      taskBuilder = std::make_shared<TaskBuilder>(_options);
      _threadPoolClient.push(taskBuilder);
    }
    if (!processTask(savedBuild))
      return false;
    if (_options._maxNumberTasks > 0 && ++numberTasks == _options._maxNumberTasks)
      break;
  } while (_options._runLoop);
  return true;
}

void Client::stop() {
  Metrics::save();
  if (auto ptr= _heartbeat.lock(); ptr)
    ptr->stop();
  _threadPoolClient.stop();
}

bool Client::printReply(const HEADER& header, std::string& buffer) {
  if (auto ptr = _heartbeat.lock(); ptr) {
    STATUS status = ptr->getStatus();
    if (utility::displayStatus(status))
      return false;
  }
  std::ostream* pstream = _options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  payloadtransform::decryptDecompress(header, buffer);
  if (buffer.empty()) {
    utility::displayStatus(STATUS::ERROR);
    return false;
  }
  stream << buffer << std::flush;
  return true;
}

void Client::start() {
  CryptoKey key;
  if (_options._showKey)
    key.showKey();
  if (!key._valid)
    throw std::runtime_error("invalid or absent crypto files.");
  if (_options._heartbeatEnabled) {
    RunnablePtr ptr = std::make_shared<tcp::TcpClientHeartbeat>(_options);
    ptr->start();
    _threadPoolClient.push(ptr);
    _heartbeat = ptr;
  }
}

void Client::onSignal() {
  _signalFlag.store(ACTIONS::ACTION);
  _signalFlag.notify_one();
}
