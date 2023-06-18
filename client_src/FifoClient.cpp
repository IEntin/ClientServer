/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "ClientOptions.h"
#include "Fifo.h"
#include "TaskBuilder.h"
#include "Utility.h"
#include "WaitSignal.h"
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <filesystem>

namespace fifo {

FifoClient::FifoClient(const ClientOptions& options) :
  Client(options) {
  try {
    boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, FIFO_NAMED_MUTEX);
    boost::interprocess::scoped_lock lock(mutex);
    if (!wakeupAcceptor())
      throw std::runtime_error("FifoClient::wakeupAcceptor failed");
    if (!receiveStatus())
      throw std::runtime_error("FifoClient::receiveStatus failed");
  }
  catch (const boost::interprocess::interprocess_exception& e) {
    LogError << e.what() << std::endl;
  }
}

FifoClient::~FifoClient() {
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
  try {
    thread_local static std::vector<char> buffer;
    HEADER header;
    if (!Fifo::readMsgBlock(_fifoName, header, buffer))
      return false;
    return printReply(header, buffer);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return false;
  }
  return true;
}

bool FifoClient::wakeupAcceptor() {
  HEADER header =
    { HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, false, _status };
  return Fifo::sendMsg(_options._acceptorName, header, _options);
}

bool FifoClient::receiveStatus() {
  try {
    HEADER header;
    std::vector<char> buffer;
    if (!Fifo::readMsgBlock(_options._acceptorName, header, buffer))
      return false;
    _clientId.assign(buffer.begin(), buffer.end());
    _status = extractStatus(header);
    _fifoName = _options._fifoDirectoryName + '/' + _clientId;
    _waitSignal = std::make_shared<WaitSignal>(_closeFlag, _fifoName, _threadPoolClient);
    _waitSignal->start();
    switch (_status) {
    case STATUS::NONE:
      break;
    case STATUS::MAX_OBJECTS_OF_TYPE:
      utility::displayMaxSessionsOfTypeWarn("fifo");
      break;
    case STATUS::MAX_TOTAL_OBJECTS:
      utility::displayMaxTotalSessionsWarn();
      break;
    default:
      break;
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return false;
  }
  return true;
}

void FifoClient::onSignal() {
  _closeFlag.store(ACTIONS::ACTION);
  _closeFlag.notify_one();
}

} // end of namespace fifo
