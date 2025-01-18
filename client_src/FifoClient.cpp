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

FifoClient::FifoClient()  {
  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, FIFO_NAMED_MUTEX);
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
    if (Fifo::sendMsg(false, _fifoName, subtask._data))
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
    _response.erase(_response.cbegin(), _response.cend());
    _status = STATUS::NONE;
    HEADER header;
    if (!Fifo::readMsgUntil(_fifoName, true, header, _response))
      return false;
    return printReply();
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
    return false;
  }
}

bool FifoClient::wakeupAcceptor() {
  auto lambda = [] (
    const HEADER& header,
    const std::string_view msgHash,
    const CryptoPP::SecByteBlock& pubKey,
    std::string_view signedAuth) {
    return Fifo::sendMsg(false, ClientOptions::_acceptorName, header, msgHash, pubKey, signedAuth);
  };
  return init(lambda);
}

bool FifoClient::receiveStatus() {
  if (_status != STATUS::NONE)
    return false;
  std::string clientIdStr;
  CryptoPP::SecByteBlock pubAreceived;
  if (!Fifo::readMsg(ClientOptions::_acceptorName, true, _header, clientIdStr, pubAreceived))
    return false;
  if (!DHFinish(clientIdStr, pubAreceived))
    return false;
  _fifoName = ClientOptions::_fifoDirectoryName + '/';
  ioutility::toChars(_clientId, _fifoName);
  startHeartbeat();
  return true;
}

} // end of namespace fifo
