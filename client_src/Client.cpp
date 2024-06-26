/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Client.h"

#include <boost/algorithm/hex.hpp>
#include <cryptopp/aes.h>

#include "ClientOptions.h"
#include "Crypto.h"
#include "Metrics.h"
#include "PayloadTransform.h"
#include "TaskBuilder.h"
#include "TcpClientHeartbeat.h"

Client::Client() :
  _dhB(Crypto::_curve),
  _privB(_dhB.PrivateKeyLength()),
  _pubB(_dhB.PublicKeyLength()),
  _generatedKeyPair(Crypto::generateKeyPair(_dhB, _privB, _pubB)),
  _pubBstring(reinterpret_cast<const char*>(_pubB.data()), _pubB.size()),
  _sharedB(_dhB.AgreedValueLength()),
  _chronometer(ClientOptions::_timing, __FILE__, __LINE__, __func__) {}

Client::~Client() {
  stop();
  _closeFlag.store(false);
  Trace << '\n';
}

// Allows to read and process the source in parts with sizes
// determined by the buffer size. This reduces memory footprint.
// For maximum speed the buffer should be large to read the
// contents in one shot.

bool Client::processTask(TaskBuilderWeakPtr weakPtr) {
  if (auto taskBuilder = weakPtr.lock(); taskBuilder) {
    static Task task;
    auto result = taskBuilder->getResult();
    auto status = std::get<0>(result);
    if (status != STATUS::NONE)
      return false;
    auto& receivedTask = std::get<1>(result);
    task.swap(receivedTask);
    taskBuilder->resume();
    for (auto& subtask : task) {
      if (!(send(subtask) && receive()))
	return false;
    }
  }
  return true;
}

bool Client::obtainKeyClientId(std::string_view buffer, const HEADER& header) {
  std::size_t payloadSz = extractPayloadSize(header);
  std::size_t pubAstringSz = extractParameter(header);
  std::string_view idStr(buffer.data(), payloadSz - pubAstringSz);
  ioutility::fromChars(idStr, _clientId);
  std::string pubAstring(buffer.data() + payloadSz - pubAstringSz, pubAstringSz);
  CryptoPP::SecByteBlock pubAreceived(reinterpret_cast<const byte*>(pubAstring.data()), pubAstring.size());
  return _dhB.Agree(_sharedB, _privB, pubAreceived);
}

void Client::run() {
  int numberTasks = 0;
  do {
    Chronometer chronometer(ClientOptions::_timing, __FILE__, __LINE__, __func__, ClientOptions::_instrStream);
    if (!processTask(_taskBuilder))
      break;
    ++numberTasks;
    if (ClientOptions::_maxNumberTasks > 0 && numberTasks == ClientOptions::_maxNumberTasks)
      break;
  } while (ClientOptions::_runLoop);
}

void Client::stop() {
  Metrics::save();
  if (auto heartbeat = _heartbeat.lock(); heartbeat)
    heartbeat->stop();
  if (auto taskBuilder = _taskBuilder.lock(); taskBuilder)
    taskBuilder->stop();
  _threadPoolClient.stop();
}

bool Client::printReply(const HEADER& header, std::string_view buffer) const {
  if (auto ptr = _heartbeat.lock(); ptr) {
    if (displayStatus(ptr->getStatus()))
      return false;
  }
  std::ostream* pstream = ClientOptions::_dataStream;
  std::ostream& stream = pstream ? *pstream : std::cout;
  std::string_view restored = payloadtransform::decryptDecompress(_sharedB, header, buffer);
  if (restored.empty()) {
    displayStatus(STATUS::ERROR);
    return false;
  }
  stream.write(restored.data(), restored.size());
  return true;
}

void Client::start() {
  if (ClientOptions::_heartbeatEnabled) {
    RunnablePtr ptr = std::make_shared<tcp::TcpClientHeartbeat>();
    ptr->start();
    _threadPoolClient.push(ptr);
    _heartbeat = ptr;
  }
  auto taskBuilder = std::make_shared<TaskBuilder>(_sharedB);
  _threadPoolClient.push(taskBuilder);
  _taskBuilder = taskBuilder;
}

void Client::onSignal(std::weak_ptr<ClientWrapper> wrapperWeak) {
  auto errnoSaved = errno;
  if (auto wrapper = wrapperWeak.lock(); wrapper) {
    Client& client = wrapper->_client;
    client.close();
  }
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
