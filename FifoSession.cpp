/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"
#include "Fifo.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "Task.h"
#include "Utility.h"
#include <filesystem>
#include <sys/stat.h>

namespace fifo {

FifoSession::FifoSession() :
  RunnableT(ServerOptions::_maxFifoSessions),
  _task(std::make_shared<Task>(_response)) {}

FifoSession::~FifoSession() {
  std::filesystem::remove(_fifoName);
  Trace << '\n';
}

bool FifoSession::start() {
  _clientId = utility::getUniqueId();
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
  Fifo::onExit(_fifoName, ServerOptions::_ENXIOwait, ServerOptions::_numberRepeatENXIO);
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
  if (serverutility::processTask(header, _request, _task))
    return sendResponse();
  return false;
}

bool FifoSession::sendResponse() {
  if (!std::filesystem::exists(_fifoName))
    return false;
  HEADER header;
  std::string_view body = serverutility::buildReply(_response, header, _status);
  if (body.empty())
    return false;
  return Fifo::sendMsg(_fifoName,
		       header,
		       ServerOptions::_ENXIOwait,
		       ServerOptions::_numberRepeatENXIO,
		       body);
}

bool FifoSession::sendStatusToClient() {
  unsigned size = _clientId.size();
  HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, _status };
  return Fifo::sendMsg(ServerOptions::_acceptorName,
		       header,
		       ServerOptions::_ENXIOwait,
		       ServerOptions::_numberRepeatENXIO,
		       _clientId);
}

} // end of namespace fifo
