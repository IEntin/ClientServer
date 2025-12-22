/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Client.h"

#include "ClientOptions.h"
#include "Metrics.h"
#include "TaskBuilder.h"
#include "TcpClientHeartbeat.h"
#include "Utility.h"

thread_local std::string Client::_buffer;

std::atomic<bool> Client::_closeFlag = false;
thread_local Subtasks Client::_task;

Client::Client() : _chronometer(ClientOptions::_timing) {
  fillEncryptorContainer(_encryptorContainer, Options::_encryptorTypeDefault);
}

Client::~Client() {
  try {
    stop();
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
  }
}

// Allows to read and process the source in parts with sizes
// determined by the buffer size. This reduces memory footprint.
// For maximum speed the buffer should be large to read the
// contents in one shot.

bool Client::processTask(TaskBuilderWeakPtr weakPtr) {
  if (auto taskBuilder = weakPtr.lock(); taskBuilder) {
    auto [size, status] = taskBuilder->getTask(_task);
    if (status == STATUS::ERROR)
      return false;
    if (ClientOptions::_runLoop)
      taskBuilder->resume();
    for (unsigned i = 0; i < size; ++i) {
      if (!(send(_task[i]) && receive()))
	return false;
    }
    return true;
  }
  return false;
}

void Client::run() {
  int numberTasks = 0;
  do {
    Chronometer chronometer(ClientOptions::_timing, ClientOptions::_instrStream);
    if (!processTask(_taskBuilder))
      break;
    ++numberTasks;
    if (ClientOptions::_maxNumberTasks > 0 && numberTasks == ClientOptions::_maxNumberTasks)
      break;
  } while (ClientOptions::_runLoop);
}

void Client::stop() {
  _status = STATUS::STOPPED;
  Metrics::save();
  if (auto heartbeat = _heartbeat.lock(); heartbeat)
    heartbeat->stop();
  if (auto taskBuilder = _taskBuilder.lock(); taskBuilder)
    taskBuilder->stop();
  _threadPoolClient.stop();
}

bool Client::printReply() {
  if (auto ptr = _heartbeat.lock(); ptr) {
    if (displayStatus(ptr->_status))
      return false;
  }
  decryptDecompress(_encryptorContainer, _buffer, _header, _response);
  std::ostream* pstream = ClientOptions::_dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  if (_response.empty()) {
    displayStatus(STATUS::ERROR);
    return false;
  }
  stream.write(&*_response.cbegin(), _response.size());
  return true;
}

void Client::start() {
  auto taskBuilder = std::make_shared<TaskBuilder>(_encryptorContainer);
  _threadPoolClient.push(taskBuilder);
  _taskBuilder = taskBuilder;
}

void Client::startHeartbeat() {
  if (ClientOptions::_heartbeatEnabled) {
    RunnablePtr heartbeat = std::make_shared<tcp::TcpClientHeartbeat>();
    heartbeat->start();
    _threadPoolClient.push(heartbeat);
    _heartbeat = heartbeat;
  }
}

void Client::onSignal() {
  auto errnoSaved = errno;
  _closeFlag.store(true);
  errno = errnoSaved;
}

void Client::displayMaxTotalSessionsWarn() const {
  Warn << "\n\t!!!!!!!!!\n"
       << "\tThe total number of sessions exceeds system capacity.\n"
       << "\tThis client will wait in the queue until load subsides.\n"
       << "\tIt will start running if some other clients are closed.\n"
       << "\tYou can also close this client and try again later,\n"
       << "\tbut spot in the queue will be lost.\n"
       << "\tSee \"MaxTotalSessions\" in ServerOptions.json.\n"
       << "\t!!!!!!!!!\n";
}

void Client::displayMaxSessionsOfTypeWarn(std::string_view type) const {
  Warn << "\n\t!!!!!!!!!\n"
       << "\tThe number of " << type << " sessions exceeds pool capacity.\n"
       << "\tThis client is waiting in the queue for available thread.\n"
       << "\tIt will start running if some other " << type << " clients\n"
       << "\tare closed.\n"
       << "\tYou can also close this client and try again later,\n"
       << "\tbut spot in the queue will be lost.\n"
       << "\tSee \"Max" << (type == "fifo" ? "Fifo" : "Tcp") << "Sessions\""
       << " in ServerOptions.json.\n"
       << "\t!!!!!!!!!\n";
}

bool Client::displayStatus(STATUS status) const {
  switch (status) {
  case STATUS::NONE:
  case STATUS::MAX_TOTAL_OBJECTS:
  case STATUS::MAX_OBJECTS_OF_TYPE:
  case STATUS::STOPPED:
  case STATUS::SESSION_STOPPED:
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
  case STATUS::ENCRYPTION_PROBLEM:
    LogError << "Encryption problem" << '\n';
    return true;
  case STATUS::DECRYPTION_PROBLEM:
    LogError << "Decryption problem" << '\n';
    return true;
  case STATUS::ERROR:
    LogError << "Internal error" << '\n';
    return true;
  default:
    LogError << "unexpected problem" << '\n';
    return true;
  }
}
