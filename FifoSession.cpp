/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"
#include "Fifo.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include <filesystem>
#include <sys/stat.h>

namespace fifo {

FifoSession::FifoSession(const ServerOptions& options) :
  RunnableT(options._maxFifoSessions, _displayType),
  _options(options) {}

FifoSession::~FifoSession() {
  std::filesystem::remove(_fifoName);
  Trace << '\n';
}

bool FifoSession::start() {
  _clientId = utility::getUniqueId();
  _fifoName = _options._fifoDirectoryName + '/' + _clientId;
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
  _request.clear();
  if (!Fifo::readMsgBlock(_fifoName, header, _request))
    return false;
  _response.clear();
  if (serverutility::processRequest(header, _request, _response))
    return sendResponse();
  return false;
}

bool FifoSession::sendResponse() {
  if (!std::filesystem::exists(_fifoName))
    return false;
  HEADER header;
  std::string_view body =
    serverutility::buildReply(_options, _response, header, _status);
  if (body.empty())
    return false;
  return Fifo::sendMsg(_fifoName, header, body);
}

bool FifoSession::sendStatusToClient() {
  size_t size = _clientId.size();
  HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, _status };
  return Fifo::sendMsg(Options::_acceptorName, header, _clientId);
}

} // end of namespace fifo
