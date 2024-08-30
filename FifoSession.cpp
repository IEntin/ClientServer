/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"

#include <filesystem>
#include <sys/stat.h>

#include "Fifo.h"
#include "IOUtility.h"
#include "Server.h"
#include "ServerOptions.h"

namespace fifo {

FifoSession::FifoSession(ServerWeakPtr server,
			 const CryptoPP::SecByteBlock& pubB) :
  RunnableT(ServerOptions::_maxFifoSessions),
  Session(server, pubB) {}

FifoSession::~FifoSession() {
  try {
    std::filesystem::remove(_fifoName);
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
  }
  Trace << '\n';
}

bool FifoSession::start() {
  _fifoName = ServerOptions::_fifoDirectoryName + '/';
  ioutility::toChars(_clientId, _fifoName);
  if (mkfifo(_fifoName.data(), 0666) == -1 && errno != EEXIST) {
    LogError << std::strerror(errno) << '-' << _fifoName << '\n';
    return false;
  }
  return true;
}

void FifoSession::run() {
  CountRunning countRunning;
  if (!std::filesystem::exists(_fifoName) || _stopped)
    return;
  while (!_stopped) {
    if (!receiveRequest())
      break;
  }
}

void FifoSession::stop() {
  _stopped = true;
  Fifo::onExit(_fifoName);
}

bool FifoSession::receiveRequest() {
  if (!std::filesystem::exists(_fifoName))
    return false;
  switch (_status) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
  case STATUS::MAX_TOTAL_OBJECTS:
    _status = STATUS::NONE;
    break;
  default:
    break;
  }
  try {
    _request.erase(0);
    if (!Fifo::readMessage(_fifoName, _request))
      return false;
    if (processTask())
      return sendResponse();
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
  }
  return false;
}

bool FifoSession::sendResponse() {
  if (!std::filesystem::exists(_fifoName))
    return false;
  std::string_view payload = buildReply(_status);
  if (payload.empty())
    return false;
  return Fifo::sendMessage(_fifoName, payload);
}

void FifoSession::sendStatusToClient() {
  if (auto server = _server.lock(); server) {
    std::string clientIdStr;
    ioutility::toChars(_clientId, clientIdStr);
    unsigned size = clientIdStr.size();
    HEADER header
      { HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, CRYPTO::NONE, DIAGNOSTICS::NONE, _status, _pubA.size() };
    Fifo::sendMsg(ServerOptions::_acceptorName, header, clientIdStr, _pubA);
  }
}

void FifoSession::displayCapacityCheck(std::atomic<unsigned>& totalNumberObjects) const {
  Session::displayCapacityCheck(totalNumberObjects,
				getNumberObjects(),
				getNumberRunningByType(),
				_maxNumberRunningByType,
				_status);
}

} // end of namespace fifo
