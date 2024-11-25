/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"

#include <filesystem>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "ClientOptions.h"
#include "Fifo.h"
#include "IOUtility.h"
#include "Subtask.h"

namespace {
constexpr int FIFO_CLIENT_POLLING_PERIOD = 250;
}

namespace fifo {

FifoClient::FifoClient() : _acceptorName(ClientOptions::_acceptorName) {
  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, utility::FIFO_NAMED_MUTEX);
  boost::interprocess::scoped_lock lock(mutex);
  if (!wakeupAcceptor())
    throw std::runtime_error("FifoClient::wakeupAcceptor failed");
  if (!receiveStatus())
    throw std::runtime_error("FifoClient::receiveStatus failed");
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
  while (true) {
    if (_closeFlag) {
      Fifo::onExit(_fifoName);
      std::error_code ec;
      std::filesystem::remove(_fifoName, ec);
      if (ec)
	LogError << ec.message() << '\n';
      _status = STATUS::STOPPED;
    }
    if (Fifo::sendMsg(_fifoName, subtask._data))
      return true;
    // waiting client
    // session stopped
    if (!std::filesystem::exists(_fifoName)) {
      _status = STATUS::SESSION_STOPPED;
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(FIFO_CLIENT_POLLING_PERIOD));
  }
  return false;
}

bool FifoClient::receive() {
  try {
    _response.erase(0);
    _status = STATUS::NONE;
    if (!Fifo::readStringBlock(_fifoName, _response))
      return false;
    return printReply();
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
    return false;
  }
}

bool FifoClient::wakeupAcceptor() {
  const auto& pubKey = _crypto->getPubKey();
  std::size_t pubKeySz = pubKey.size();
  std::string_view serializedRsaKey = _crypto->getSerializedRsaPubKey();
  std::size_t rsaStrSz = serializedRsaKey.size();
  HEADER header =
    { HEADERTYPE::DH_INIT, pubKeySz, COMPRESSORS::NONE, DIAGNOSTICS::NONE, _status, rsaStrSz };
  return Fifo::sendMsg(_acceptorName, header, pubKey, serializedRsaKey);
}

bool FifoClient::receiveStatus() {
  if (_status != STATUS::NONE)
    return false;
  std::string clientIdStr;
  CryptoPP::SecByteBlock pubAreceived;
  if (!Fifo::readMsg(_acceptorName, true, _header, clientIdStr, pubAreceived))
    return false;
  if (!_crypto->handshake(pubAreceived)) {
    LogError << "handshake failed";
    return false;
  }
  ioutility::fromChars(clientIdStr, _clientId);
  _status = extractStatus(_header);
  _fifoName = ClientOptions::_fifoDirectoryName + '/';
  ioutility::toChars(_clientId, _fifoName);
  switch (_status) {
  case STATUS::NONE:
    break;
  case STATUS::MAX_OBJECTS_OF_TYPE:
    displayMaxSessionsOfTypeWarn("fifo");
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    displayMaxTotalSessionsWarn();
    break;
  default:
    return false;
    break;
  }
  return true;
}

} // end of namespace fifo
