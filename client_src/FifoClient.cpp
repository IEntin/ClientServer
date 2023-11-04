/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "ClientOptions.h"
#include "Fifo.h"
#include "Logger.h"
#include "Subtask.h"
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <filesystem>

namespace fifo {

FifoClient::FifoClient() {
  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, FIFO_NAMED_MUTEX);
  boost::interprocess::scoped_lock lock(mutex);
  if (!wakeupAcceptor())
    return;
  receiveStatus();
}

FifoClient::~FifoClient() {
  Fifo::onExit(_fifoName);
  Trace << '\n';
}

bool FifoClient::run() {
  start();
  return Client::run();
}

bool FifoClient::send(const Subtask& subtask) {
  std::string_view body(subtask._body.data(), subtask._body.size());
  while (true) {
    if (_signalFlag) {
      Fifo::onExit(_fifoName);
      std::error_code ec;
      std::filesystem::remove(_fifoName, ec);
      if (ec)
	LogError << ec.message() << '\n';
    }
    if (Fifo::sendMsg(_fifoName, subtask._header, body))
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
    { HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, false, _status };
  return Fifo::sendMsg(ClientOptions::_acceptorName, header);
}

bool FifoClient::receiveStatus() {
  HEADER header;
  std::string buffer;
  if (!Fifo::readMsgBlock(ClientOptions::_acceptorName, header, buffer))
    return false;
  _clientId.assign(buffer);
  _status = extractStatus(header);
  _fifoName = ClientOptions::_fifoDirectoryName + '/' + _clientId;
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
  _signalFlag.store(true);
}

} // end of namespace fifo
