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
			 std::string_view pubBstring) :
  RunnableT(ServerOptions::_maxFifoSessions),
  Session(server, pubBstring) {}

FifoSession::~FifoSession() {
  std::filesystem::remove(_fifoName);
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
  while (true) {
    HEADER header;
    if (!receiveRequest(header))
      break;
  }
}

void FifoSession::stop() {
  _stopped = true;
  Fifo::onExit(_fifoName);
}

bool FifoSession::receiveRequest(HEADER& header) {
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
  if (!Fifo::readMsgBlock(_fifoName, header, _request))
    return false;
  if (processTask(header))
    return sendResponse();
  return false;
}

bool FifoSession::sendResponse() {
  if (!std::filesystem::exists(_fifoName))
    return false;
  HEADER header;
  std::string_view body = buildReply(header, _status);
  if (body.empty())
    return false;
  return Fifo::sendMsg(_fifoName, header, body);
}

void FifoSession::sendStatusToClient() {
  if (auto server = _server.lock(); server) {
    std::string payload;
    ioutility::toChars(_clientId, payload);
    payload.append(reinterpret_cast<const char*>(_pubA.data()), _pubA.size());
    unsigned size = payload.size();
    HEADER header
      { HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, _status, _pubA.size() };
    Fifo::sendMsg(ServerOptions::_acceptorName, header, payload);
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
