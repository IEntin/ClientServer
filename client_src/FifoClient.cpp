/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "ClientOptions.h"
#include "Fifo.h"
#include "TaskBuilder.h"
#include "Utility.h"
#include "SignalWatcher.h"
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <filesystem>

namespace fifo {

FifoClient::FifoClient(const ClientOptions& options) :
  Client(options) {
  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, FIFO_NAMED_MUTEX);
  boost::interprocess::scoped_lock lock(mutex);
  if (!wakeupAcceptor())
    return;
  receiveStatus();
}

FifoClient::~FifoClient() {
  if (auto ptr = _signalWatcher.lock(); ptr)
    ptr->stop();
  Fifo::onExit(_fifoName, _options);
  Trace << std::endl;
}

bool FifoClient::run() {
  start();
  return Client::run();
}

bool FifoClient::send(const Subtask& subtask) {
  std::string_view body(subtask._body.data(), subtask._body.size());
  while (true) {
    if (Fifo::sendMsg(_fifoName, subtask._header, _options, body))
      return true;
    // waiting client
    // server stopped
    if (!std::filesystem::exists(_fifoName))
      return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(FIFO_CLIENT_POLLING_PERIOD));
  }
  return false;
}

bool FifoClient::receive() {
  if (!std::filesystem::exists(_options._controlFileName))
    return false;
  _status = STATUS::NONE;
  thread_local static std::vector<char> buffer;
  HEADER header;
  if (!Fifo::readMsgBlock(_fifoName, header, buffer))
    return false;
  return printReply(header, buffer);
}

bool FifoClient::wakeupAcceptor() {
  HEADER header =
    { HEADERTYPE::CREATE_SESSION, 0, 0, 0, COMPRESSORS::NONE, false, false, _status };
  return Fifo::sendMsg(_options._acceptorName, header, _options);
}

bool FifoClient::receiveStatus() {
  HEADER header;
  std::vector<char> buffer;
  if (!Fifo::readMsgBlock(_options._acceptorName, header, buffer))
    return false;
  _clientId.assign(buffer.begin(), buffer.end());
  _status = extractStatus(header);
  _fifoName = _options._fifoDirectoryName + '/' + _clientId;
  createSignalWatcher();
  switch (_status) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    utility::displayMaxSessionsOfTypeWarn("fifo");
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    utility::displayMaxTotalSessionsWarn();
    break;
  default:
    break;
  }
  return true;
}

void FifoClient::createSignalWatcher() {
  const ClientOptions& options(_options);
  std::string& name(_fifoName);
  std::function<void()> func = [&options, name]() {
    Fifo::onExit(name, options);
    std::filesystem::remove(name);
  };
  auto ptr = std::make_shared<SignalWatcher>(_signalFlag, func);
  _signalWatcher = ptr;
  _threadPoolClient.push(ptr);
}

} // end of namespace fifo
