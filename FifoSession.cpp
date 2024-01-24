/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"

#include <cassert>
#include <filesystem>
#include <sys/stat.h>

#include <boost/algorithm/hex.hpp>

#include "DHKeyExchange.h"
#include "Fifo.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "ServerUtility.h"
#include "Task.h"
#include "Utility.h"

namespace fifo {

FifoSession::FifoSession() :
  RunnableT(ServerOptions::_maxFifoSessions),
  _task(std::make_shared<Task>(_response)) {
  _Astring = DHKeyExchange::step1(_priv, _pub);
}

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

bool FifoSession::receiveBString() {
  HEADER header;
  std::string Bstring;
  if (!Fifo::readMsgBlock(_fifoName, header, Bstring))
    return false;
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
  std::string body;
  if (!Fifo::readMsgBlock(_fifoName, header, body))
    return false;
  auto&  [type, payloadSz, uncomprSz, compressor, encrypted, diagnostics, status, parameter] = header;
  assert(payloadSz == body.size());
  if (type == HEADERTYPE::KEY_EXCHANGE) {
    std::string Bstring = body.substr(payloadSz - parameter, parameter);
    CryptoPP::Integer crossPub(Bstring.c_str());
    _key = DHKeyExchange::step2(_priv, crossPub);
    body.erase(payloadSz - parameter, parameter);
    header =
      { HEADERTYPE::SESSION, payloadSz - parameter, uncomprSz, compressor, encrypted, diagnostics, status, 0 };
  }
  _request.swap(body);
  if (serverutility::processTask(_key, header, _request, _task))
    return sendResponse();
  return false;
}

bool FifoSession::sendResponse() {
  if (!std::filesystem::exists(_fifoName))
    return false;
  HEADER header;
  std::string_view body = serverutility::buildReply(_key, _response, header, _status);
  if (body.empty())
    return false;
  return Fifo::sendMsg(_fifoName, header, body);
}

bool FifoSession::sendStatusToClient() {
  std::string payload(_clientId);
  payload.append(1, '\n').append(_Astring);
  unsigned size = payload.size();
  HEADER header{ HEADERTYPE::CREATE_SESSION, size, size, COMPRESSORS::NONE, false, false, _status, 0 };
  return Fifo::sendMsg(ServerOptions::_acceptorName, header, payload);
}

} // end of namespace fifo
