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
    if (displayStatus(status))
      return false;
  }
  std::ostream* pstream = _options._dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  payloadtransform::decryptDecompress(header, buffer);
  if (buffer.empty()) {
    displayStatus(STATUS::ERROR);
    return false;
  }
  stream << buffer << std::flush;
  return true;
}

void Client::start() {
  CryptoKey::recover();
  if (_options._showKey)
    CryptoKey::showKey();
  if (!CryptoKey::_valid)
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

void Client::displayMaxTotalSessionsWarn() {
  Warn << "\n\t!!!!!!!!!\n"
       << "\tThe total number of sessions exceeds system capacity.\n"
       << "\tThis client will wait in the queue until load subsides.\n"
       << "\tIt will start running if some other clients are closed.\n"
       << "\tYou can also close this client and try again later,\n"
       << "\tbut spot in the queue will be lost.\n"
       << "\tSee \"MaxTotalSessions\" in ServerOptions.json.\n"
       << "\t!!!!!!!!!" << '\n';
}

void Client::displayMaxSessionsOfTypeWarn(std::string_view type) {
  Warn << "\n\t!!!!!!!!!\n"
       << "\tThe number of " << type << " sessions exceeds pool capacity.\n"
       << "\tThis client is waiting in the queue for available thread.\n"
       << "\tIt will start running if some other " << type << " clients\n"
       << "\tare closed.\n"
       << "\tYou can also close this client and try again later,\n"
       << "\tbut spot in the queue will be lost.\n"
       << "\tSee \"Max" << (type == "fifo" ? "Fifo" : "Tcp") << "Sessions\""
       << " in ServerOptions.json.\n"
       << "\t!!!!!!!!!" << '\n';
}

bool Client::displayStatus(STATUS status) {
  switch (status) {
  case STATUS::NONE:
    return false;
  case STATUS::BAD_HEADER:
    LogError << "STATUS::BAD_HEADER" << '\n';
    return true;
  case STATUS::COMPRESSION_PROBLEM:
    LogError << "STATUS::COMPRESSION_PROBLEM" << '\n';
    return true;
  case STATUS::DECOMPRESSION_PROBLEM:
    LogError << "STATUS::DECOMPRESSION_PROBLEM" << '\n';
    return true;
  case STATUS::FIFO_PROBLEM:
    LogError << "STATUS::FIFO_PROBLEM" << '\n';
    return true;
  case STATUS::TCP_PROBLEM:
    LogError << "STATUS::TCP_PROBLEM" << '\n';
    return true;
  case STATUS::TCP_TIMEOUT:
    LogError << "\tserver timeout! Increase \"TcpTimeout\" in ServerOptions.json" << '\n';
    return true;
 case STATUS::HEARTBEAT_PROBLEM:
    LogError << "STATUS::HEARTBEAT_PROBLEM" << '\n';
    return true;
 case STATUS::HEARTBEAT_TIMEOUT:
    LogError << "\theartbeat timeout! Increase \"HeartbeatTimeout\" in ClientOptions.json" << '\n';
    return true;
  case STATUS::MAX_TOTAL_OBJECTS:
    Warn << "Exceeded max total number clients" << '\n';
    return false;
  case STATUS::MAX_OBJECTS_OF_TYPE:
    Warn << "Exceeded max number clients of type" << '\n';
    return false;
  case STATUS::ENCRYPTION_PROBLEM:
    LogError << "Encryption problem" << '\n';
    return false;
  case STATUS::DECRYPTION_PROBLEM:
    LogError << "Decryption problem" << '\n';
    return false;
  case STATUS::ERROR:
    LogError << "Internal error" << '\n';
    return false;
  default:
    LogError << "unexpected problem" << '\n';
    return true;
  }
}
