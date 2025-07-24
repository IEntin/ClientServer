/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"

#include <filesystem>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "ClientOptions.h"
#include "Fifo.h"
#include "Options.h"
#include "Utility.h"

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
  _response.resize(ClientOptions::_bufferSize);
}

FifoClient::~FifoClient() {
  try {
  if (std::filesystem::exists(_fifoName))
    Fifo::onExit(_fifoName);
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
  }
}

void FifoClient::run() {
  start();
  Client::run();
}

bool FifoClient::send(const Subtask& subtask) {
  while (true) {
    if (_closeFlag) {
      Fifo::onExit(_fifoName);
      std::error_code ec;
      std::filesystem::remove(_fifoName, ec);
      if (ec)
	LogError << ec.message() << '\n';
      _status = STATUS::STOPPED;
    }
    if (Fifo::sendMessage(false, _fifoName, subtask._header, subtask._data))
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
    _status = STATUS::NONE;
    HEADER header;
    if (!Fifo::readMessage(_fifoName, true, header, _response))
      return false;
    return printReply();
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
    return false;
  }
}

bool FifoClient::wakeupAcceptor() {
  auto lambda = []<typename T1, typename T2, typename T3> (
    const HEADER& header,
    const T1& msgHash,
    const T2& pubKeyAesServer,
    const T3& signedAuth) {
    return Fifo::sendMessage(false, Options::_acceptorName, header, msgHash, pubKeyAesServer, signedAuth);
  };
  return _crypto->sendSignature(lambda);
}

bool FifoClient::receiveStatus() {
  if (_status != STATUS::NONE)
    return false;
  std::string clientIdStr;
  std::vector<unsigned char> pubKeyAesServer;
  if (!Fifo::readMessage(Options::_acceptorName, true, _header, clientIdStr, pubKeyAesServer))
    return false;
  if (!DHFinish(clientIdStr, pubKeyAesServer))
    return false;
  _fifoName = Options::_fifoDirectoryName + '/';
  ioutility::toChars(_clientId, _fifoName);
  startHeartbeat();
  return true;
}

} // end of namespace fifo
