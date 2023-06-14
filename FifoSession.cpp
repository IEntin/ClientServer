/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"
#include "Fifo.h"
#include "MemoryPool.h"
#include "Server.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "ThreadPoolDiffObj.h"
#include "Utility.h"
#include <filesystem>

namespace fifo {

FifoSession::FifoSession(Server& server, std::string_view clientId) :
  RunnableT(server.getOptions()._maxFifoSessions),
  _options(server.getOptions()),
  _clientId(clientId),
  _fifoName(_options._fifoDirectoryName + '/' + _clientId),
  _threadPool(server.getThreadPoolSession()) {
  Debug << "_fifoName:" << _fifoName << std::endl;
}

FifoSession::~FifoSession() {
  try {
    std::filesystem::remove(_fifoName);
    Trace << std::endl;
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
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

void FifoSession::checkCapacity() {
  Info << "Number fifo sessions=" << _numberObjects
       << ", max number fifo running=" << _maxNumberRunningByType
       << std::endl;
  switch (_status) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    Warn << "\nThe number of fifo sessions=" << _numberObjects
	 << " exceeds thread pool capacity." << std::endl;
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    Warn << "\nTotal sessions=" << _threadPool.numberRelatedObjects()
	 << " exceeds system capacity." << std::endl;
    break;
  default:
    break;
  }
}

bool FifoSession::start() {
  _threadPool.push(shared_from_this());
  return true;
}

void FifoSession::stop() {
  _stopped = true;
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
  try {
    static thread_local std::vector<char> request;
    request.clear();
    if (!Fifo::readMsgBlock(_fifoName, header, request))
      return false;
    static thread_local Response response;
    response.clear();
    if (serverutility::processRequest(header, request, response))
      return sendResponse(response);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    MemoryPool::destroyBuffers();
    return false;
  }
  return true;
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
