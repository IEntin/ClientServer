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
		 std::string_view msgHash,
		 const CryptoPP::SecByteBlock& pubB,
		 std::string_view signatureWithPubKey) :
  _crypto(std::make_shared<Crypto>(msgHash, pubB, signatureWithPubKey)),
  _task(std::make_shared<Task>(_response)),
  _server(server) {
  _clientId = utility::getUniqueId();
  std::string signature(signatureWithPubKey.data(), RSA_KEY_SIZE >> 3);
  std::string_view rsaPubKeySerialized(
    signatureWithPubKey.cbegin() + (RSA_KEY_SIZE >> 3), signatureWithPubKey.cend());
  _crypto->decodePeerRsaPublicKey(rsaPubKeySerialized);
  if (!_crypto->verifySignature(signature))
    throw std::runtime_error("signature verification failed.");
  _crypto->hideKey();
  if (ServerOptions::_showKey)
   _crypto->showKey();
  _crypto->eraseRSAKeys();
}

Session::~Session() {
  Trace << '\n';
}

std::pair<HEADER, std::string_view>
Session::buildReply(std::atomic<STATUS>& status) {
  if (_response.empty())
    return {};
  _responseData.erase(_responseData.cbegin(), _responseData.cend());
  for (const auto& entry : _response)
    _responseData.insert(_responseData.end(), entry.cbegin(), entry.cend());
  if (ServerOptions::_showKey)
    _crypto->showKey();
  HEADER header =
    { HEADERTYPE::SESSION, 0,_responseData.size(), ServerOptions::_compressor, DIAGNOSTICS::NONE, status, 0 };
  utility::compressEncrypt(_buffer, ServerOptions::_encrypted, header, _crypto, _responseData);
  header =
    { HEADERTYPE::SESSION, 0, _responseData.size(), ServerOptions::_compressor, DIAGNOSTICS::NONE, status, 0 };
  return { header, _responseData };
}

bool Session::processTask() {
  utility::decryptDecompress(_buffer, _task->header(), _crypto, _request);
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
