/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"

#include <filesystem>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "ClientOptions.h"
#include "Fifo.h"
#include "Logger.h"
#include "Subtask.h"

namespace fifo {

FifoClient::FifoClient() : _acceptorName(ClientOptions::_acceptorName) {
  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, FIFO_NAMED_MUTEX);
  boost::interprocess::scoped_lock lock(mutex);
  if (!wakeupAcceptor())
    return;
  receiveStatus();
}

FifoClient::~FifoClient() {
  if (std::filesystem::exists(_fifoName))
    Fifo::onExit(_fifoName);
  Trace << '\n';
}

void FifoClient::run() {
  start();
  Client::run();
}

bool FifoClient::send(Subtask& subtask) {
  packBstring(subtask);
  while (true) {
    if (_closeFlag) {
      Fifo::onExit(_fifoName);
      std::error_code ec;
      std::filesystem::remove(_fifoName, ec);
      if (ec)
	LogError << ec.message() << '\n';
    }
    if (Fifo::sendMsg(_fifoName, subtask._header, subtask._body))
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
  _status = STATUS::NONE;
  _response.clear();
  HEADER header;
  if (!Fifo::readMsgBlock(_fifoName, header, _response))
    return false;
  return printReply(header, _response);
}

bool FifoClient::wakeupAcceptor() {
  HEADER header =
    { HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, false, _status, 0 };
  return Fifo::sendMsg(_acceptorName, header);
}

bool FifoClient::receiveStatus() {
  HEADER header;
  std::string buffer;
  if (!Fifo::readMsgBlock(_acceptorName, header, buffer))
    return false;
  if (!obtainKeyClientId(buffer, header))
    return false;
  _status = extractStatus(header);
  _fifoName = ClientOptions::_fifoDirectoryName + '/';
  ioutility::toChars(_clientId, _fifoName);
  switch (_status) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    displayMaxSessionsOfTypeWarn("fifo");
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    displayMaxTotalSessionsWarn();
    break;
  default:
    break;
  }
  return true;
}

void FifoClient::close() {
  _closeFlag.store(true);
}

} // end of namespace fifo
