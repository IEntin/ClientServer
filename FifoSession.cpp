/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"

#include <filesystem>
#include <sys/stat.h>

#include "Fifo.h"
#include "Server.h"
#include "ServerOptions.h"

namespace fifo {

FifoSession::FifoSession(ServerWeakPtr server,
			 const CryptoPP::SecByteBlock& pubB,
			 std::span<uint8_t> signatureWithPubKey) :
  RunnableT(ServerOptions::_maxFifoSessions),
  Session(server, pubB, signatureWithPubKey) {}

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
    LogError << strerror(errno) << '-' << _fifoName << '\n';
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
    _request.erase(_request.cbegin(), _request.cend());
    if (!Fifo::readStringBlock(_fifoName, _request))
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
  return Fifo::sendMsg(_fifoName, payload);
}

void FifoSession::sendStatusToClient() {
  auto lambda = [] (
    const HEADER& header, std::string_view idStr, const CryptoPP::SecByteBlock& pubA) {
    Fifo::sendMsg(ServerOptions::_acceptorName, header, idStr, pubA);
  };
  Session::sendStatusToClient(lambda, _status);
}

void FifoSession::displayCapacityCheck(std::atomic<unsigned>& totalNumberObjects) const {
  Session::displayCapacityCheck(totalNumberObjects,
				getNumberObjects(),
				getNumberRunningByType(),
				_maxNumberRunningByType,
				_status);
}

} // end of namespace fifo
