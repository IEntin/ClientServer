/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Session.h"

#include "Logger.h"
#include "Server.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"

Session::Session(ServerWeakPtr server,
		 const CryptoPP::SecByteBlock& pubB,
		 std::string_view signatureWithPubKey) :
  _crypto(std::make_shared<Crypto>(pubB)),
  _task(std::make_shared<Task>(_response)),
  _server(server) {
  _clientId = utility::getUniqueId();
  std::string signature(signatureWithPubKey.data(), SIGNATURE_SIZE);
  std::string_view rsaPubKeySerialized(
    signatureWithPubKey.cbegin() + SIGNATURE_SIZE, signatureWithPubKey.cend());
  _crypto->decodePeerRsaPublicKey(rsaPubKeySerialized);
  if (!_crypto->verifySignature(signature))
    throw std::runtime_error("signature verification failed.");
  _crypto->hideSessionKey();
  if (ServerOptions::_showKey)
   _crypto->showKey();
}

Session::~Session() {
  Trace << '\n';
}

std::string_view Session::buildReply(std::atomic<STATUS>& status) {
  if (_response.empty())
    return {};
  _responseData.erase(_responseData.cbegin(), _responseData.cend());
  for (const auto& entry : _response)
    _responseData.insert(_responseData.end(), entry.cbegin(), entry.cend());
  std::size_t uncompressedSz = _responseData.size();
  HEADER header =
    { HEADERTYPE::SESSION, uncompressedSz, ServerOptions::_compressor, DIAGNOSTICS::NONE, status, 0 };
  if (ServerOptions::_showKey)
    _crypto->showKey();
  _crypto->recoverSessionKey();
  utility::compressEncrypt(_buffer, ServerOptions::_encrypted, header, _crypto, _responseData);
  _crypto->hideSessionKey();
  return _responseData;
}

bool Session::processTask() {
  _crypto->recoverSessionKey();
  utility::decryptDecompress(_buffer, _task->header(), _crypto, _request);
  _crypto->hideSessionKey();
   if (auto taskController = TaskController::getWeakPtr().lock(); taskController) {
    std::string_view request = _request;
    _task->update(request);
    taskController->processTask(_task);
    return true;
  }
  return false;
}

void Session::displayCapacityCheck(unsigned totalNumberObjects,
				   unsigned numberObjects,
				   unsigned numberRunningByType,
				   unsigned maxNumberRunningByType,
				   STATUS status) const {
  Info << "Number " << getDisplayName() << " sessions=" << numberObjects
       << ", Number running " << getDisplayName() << " sessions=" << numberRunningByType
       << ", max number " << getDisplayName() << " running=" << maxNumberRunningByType
       << '\n';
  switch (status) {
  case STATUS::MAX_OBJECTS_OF_TYPE:
    Warn << "\nThe number of " << getDisplayName() << " sessions="
	 << numberObjects << " exceeds thread pool capacity." << '\n';
    break;
  case STATUS::MAX_TOTAL_OBJECTS:
    Warn << "\nTotal sessions=" << totalNumberObjects
	 << " exceeds system capacity." << '\n';
    break;
  default:
    break;
  }
}
