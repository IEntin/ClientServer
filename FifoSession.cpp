/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"
#include "Fifo.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include <filesystem>

namespace fifo {

FifoSession::FifoSession(const ServerOptions& options, std::string_view clientId) :
  RunnableT(options._maxFifoSessions, _name),
  _options(options),
  _clientId(clientId),
  _fifoName(_options._fifoDirectoryName + '/' + _clientId) {
  Debug << "_fifoName:" << _fifoName << '\n';
}

FifoSession::~FifoSession() {
  std::filesystem::remove(_fifoName);
  Trace << '\n';
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
  Fifo::onExit(_fifoName, _options);
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
  static thread_local std::string request;
  request.clear();
  if (!Fifo::readMsgBlock(_fifoName, header, request))
    return false;
  static thread_local Response response;
  response.clear();
  if (serverutility::processRequest(header, request, response))
    return sendResponse(response);
  return false;
}

bool FifoSession::sendResponse(const Response& response) {
  if (!std::filesystem::exists(_fifoName))
    return false;
  HEADER header;
  std::string_view body =
    serverutility::buildReply(_options, response, header, _status);
  if (body.empty())
    return false;
  return Fifo::sendMsg(_fifoName, header, _options, body);
}

bool FifoSession::sendStatusToClient() {
  size_t size = _clientId.size();
  HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, _status };
  return Fifo::sendMsg(_options._acceptorName, header, _options, _clientId);
}

} // end of namespace fifo
