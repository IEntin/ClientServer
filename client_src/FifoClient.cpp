/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"

#include <filesystem>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "ClientOptions.h"
#include "Options.h"
#include "Utility.h"

namespace {
constexpr int FIFO_CLIENT_POLLING_PERIOD = 250;
}

namespace fifo {

FifoClient::FifoClient()  {
  boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, FIFO_NAMED_MUTEX);
  boost::interprocess::scoped_lock lock(mutex);
  bool success;
  success = wakeupAcceptor(_encryptorVariant);
  if (!success)
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
    std::array<std::reference_wrapper<std::string>, 1> array{ std::ref(_response) };
    if (!Fifo::readMessage(_fifoName, true, header, array))
      return false;
    return printReply();
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
    return false;
  }
}

bool FifoClient::receiveStatus() {
  if (_status != STATUS::NONE)
    return false;
  std::string clientIdStr;
  std::string encodedPeerPubKeyAes;
  std::string type;
  auto lambda = [this] (HEADER& header,
			std::string& clientIdStr,
			std::string& encodedPeerPubKeyAes,
			std::string& type) -> bool {
    std::array<std::reference_wrapper<std::string>, 2> array{ std::ref(clientIdStr),
							      std::ref(encodedPeerPubKeyAes) };
    if (!Fifo::readMessage(Options::_acceptorName, true, header, array))
      throw std::runtime_error("readMessage failed");
    type = "fifo";
    _fifoName = Options::_fifoDirectoryName + '/' + clientIdStr;
    ioutility::fromChars(clientIdStr, _clientId);
    return true;
  };
  if (!lambda(_header, clientIdStr, encodedPeerPubKeyAes, type))
    return false;
  return processStatus(encodedPeerPubKeyAes, type);
}

} // end of namespace fifo
