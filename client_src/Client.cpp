/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Client.h"
#include "ClientOptions.h"
#include "Crypto.h"
#include "PayloadTransform.h"
#include "Metrics.h"
#include "TaskBuilder.h"
#include "TcpClientHeartbeat.h"

Client::Client() :
  _chronometer(ClientOptions::_timing, __FILE__, __LINE__, __func__) {}

Client::~Client() {
  stop();
  _signalFlag.store(false);
  Trace << '\n';
}

// Allows to read and process the source in parts with sizes
// determined by the buffer size. This reduces memory footprint.
// For maximum speed the buffer should be large to read the
// content in one shot.

bool Client::processTask(TaskBuilderWeakPtr weakPtr) {
  if (auto taskBuilder = weakPtr.lock(); taskBuilder) {
    while (true) {
      Subtask& subtask = taskBuilder->getSubtask();
      STATUS state = subtask._state;
      switch (state) {
      case STATUS::ERROR:
	LogError << "TaskBuilder failed." << '\n';
	return false;
      case STATUS::SUBTASK_DONE:
      case STATUS::TASK_DONE:
	if (!(send(subtask) && receive()))
	  return false;
	if (state == STATUS::TASK_DONE) {
	  taskBuilder->resume();
	  return true;
	}
	break;
      default:
	return false;
      }
    }
  }
  return false;
}

bool Client::run() {
  int numberTasks = 0;
  do {
    Chronometer chronometer(ClientOptions::_timing, __FILE__, __LINE__, __func__, ClientOptions::_instrStream);
    auto current = numberTasks % 2 == 1 ? _taskBuilder1 : _taskBuilder2;
    if (!processTask(current))
      return false;
    ++numberTasks;
    if (ClientOptions::_maxNumberTasks > 0 && numberTasks == ClientOptions::_maxNumberTasks)
      break;
  } while (ClientOptions::_runLoop);
  return true;
}

void Client::stop() {
  Metrics::save();
  if (auto heartbeat = _heartbeat.lock(); heartbeat)
    heartbeat->stop();
  if (auto taskBuilder = _taskBuilder1.lock(); taskBuilder)
    taskBuilder->stop();
  if (auto taskBuilder = _taskBuilder2.lock(); taskBuilder)
    taskBuilder->stop();
  _threadPoolClient.stop();
}

bool Client::printReply(const HEADER& header, std::string_view buffer) {
  if (auto ptr = _heartbeat.lock(); ptr) {
    STATUS status = ptr->getStatus();
    if (displayStatus(status))
      return false;
  }
  std::ostream* pstream = ClientOptions::_dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  std::string_view restored = payloadtransform::decryptDecompress(header, buffer);
  if (restored.empty()) {
    displayStatus(STATUS::ERROR);
    return false;
  }
  stream.write(restored.data(), restored.size());
  return true;
}

void Client::start() {
  CryptoKey::recover();
  if (ClientOptions::_showKey)
    CryptoKey::showKey();
  if (!CryptoKey::_valid)
    throw std::runtime_error("invalid or absent crypto files.");
  if (ClientOptions::_heartbeatEnabled) {
    RunnablePtr ptr = std::make_shared<tcp::TcpClientHeartbeat>();
    ptr->start();
    _threadPoolClient.push(ptr);
    _heartbeat = ptr;
  }
  auto taskBuilder1 = std::make_shared<TaskBuilder>();
  _threadPoolClient.push(taskBuilder1);
  _taskBuilder1 = taskBuilder1;
  auto taskBuilder2 = std::make_shared<TaskBuilder>();
  _threadPoolClient.push(taskBuilder2);
  _taskBuilder2 = taskBuilder2;
}

void Client::onSignal(std::atomic<Client*>& clientPtr) {
  auto errnoSaved = errno;
  if (clientPtr)
    clientPtr.load()->close();
  errno = errnoSaved;
}

void Client::displayMaxTotalSessionsWarn() {
  Warn << "\n\t!!!!!!!!!\n"
       << "\tThe total number of sessions exceeds system capacity.\n"
       << "\tThis client will wait in the queue until load subsides.\n"
       << "\tIt will start running if some other clients are closed.\n"
       << "\tYou can also close this client and try again later,\n"
       << "\tbut spot in the queue will be lost.\n"
       << "\tSee \"MaxTotalSessions\" in ServerOptions.json.\n"
       << "\t!!!!!!!!!\n";
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
       << "\t!!!!!!!!!\n";
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
