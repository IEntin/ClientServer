/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"

#include <cassert>
#include <filesystem>
#include <sys/stat.h>

#include "Fifo.h"
#include "Logger.h"
#include "ServerOptions.h"

namespace fifo {

FifoSession::FifoSession() :
  RunnableT(ServerOptions::_maxFifoSessions) {}

FifoSession::~FifoSession() {
  std::filesystem::remove(_fifoName);
  Trace << '\n';
}

bool FifoSession::start() {
  _fifoName = ServerOptions::_fifoDirectoryName + '/' + _clientId;
  if (mkfifo(_fifoName.data(), 0666) == -1 && errno != EEXIST) {
    LogError << std::strerror(errno) << '-' << _fifoName << '\n';
    return false;
  }
  return true;
}

void FifoSession::run() {
  CountRunning countRunning;
  if (!std::filesystem::exists(_fifoName))
    return;
  if (_stopped)
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
  createKey(header);
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

bool FifoSession::sendStatusToClient() {
  std::string payload(_clientId);
  payload.append(_Astring);
  unsigned size = payload.size();
  HEADER header
    { HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, _status, _Astring.size() };
  return Fifo::sendMsg(ServerOptions::_acceptorName, header, payload);
}

} // end of namespace fifo
