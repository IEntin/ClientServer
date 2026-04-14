/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoSession.h"

#include <filesystem>
#include <sys/stat.h>

#include <boost/stacktrace.hpp>

#include "Fifo.h"
#include "Server.h"
#include "ServerOptions.h"

namespace fifo {

static constexpr auto TYPE{ "fifo" };

FifoSession::FifoSession(ServerWeakPtr server,
			 std::string_view encodedPeerPubKeyAes,
			 std::string_view signatureWithPubKey) :
  RunnableT(ServerOptions::_maxFifoSessions),
  Session(server, encodedPeerPubKeyAes, signatureWithPubKey) {}

FifoSession::~FifoSession() {
  try {
    std::filesystem::remove(_fifoName);
  }
  catch (const std::exception& e) {
    Warn << e.what() << '\n';
  }
}

bool FifoSession::start() {
  _fifoName = Options::_fifoDirectoryName + '/';
  _fifoName += ioutility::toCharsBoost(_clientId);
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
    _request.clear();
    HEADER header;
    std::array<std::reference_wrapper<std::string>, 1> array{std::ref(_request)};
    if (!Fifo::readMessage(_fifoName, true, header, array))
      return false;
    if (processTask())
      return sendResponse();
  }
  catch (const std::exception& e) {
    Warn << boost::stacktrace::stacktrace() << '\n';
    Warn << e.what() << '\n';
  }
  return false;
}

bool FifoSession::sendResponse() {
  if (!std::filesystem::exists(_fifoName))
    return false;
  auto [header, payload] = buildReply(_status);
  if (payload.empty())
    return false;
  return Fifo::sendMessage(false, _fifoName, header, payload);
}

void FifoSession::sendStatusToClient() {
  auto lambda = [] (const HEADER& header,
		    std::string_view idStr,
		    std::string_view pubKey) {
    Fifo::sendMessage(false, Options::_acceptorName, header, idStr, pubKey);
  };
  Session::sendStatusToClient(lambda, _status);
}

void FifoSession::displayCapacityCheck(std::atomic<unsigned>& totalNumberObjects) {
  Session::displayCapacityCheck(TYPE,
				totalNumberObjects,
				getNumberObjects(),
				getNumberRunningByType(),
				_maxNumberRunningByType,
				_status);
  sendStatusToClient();
}

} // end of namespace fifo
