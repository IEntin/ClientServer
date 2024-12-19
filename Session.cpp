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
		 std::span<uint8_t> signatureWithPubKey) :
  _crypto(std::make_shared<Crypto>(pubB)),
  _task(std::make_shared<Task>(_response)),
  _server(server) {
  _clientId = utility::getUniqueId();
  std::span<uint8_t, SIGNATURE_SIZE> signature(signatureWithPubKey);
  std::span<uint8_t> rsaPubKeySerialized = signatureWithPubKey.subspan(SIGNATURE_SIZE);
  auto rsaPeerPublicKey = _crypto->deserializeRsaPublicKey(rsaPubKeySerialized);
  bool verified = _crypto->verifySignature(signature, *rsaPeerPublicKey);
  if (!verified)
    throw std::runtime_error("signature verification failed.");
}

Session::~Session() {
  Trace << '\n';
}

std::string_view Session::buildReply(std::atomic<STATUS>& status) {
  if (_response.empty())
    return {};
  _responseData.erase(_responseData.cbegin(), _responseData.cend());
  for (const auto& entry : _response)
    _responseData.insert(_responseData.end(), entry.begin(), entry.end());
  std::size_t uncompressedSz = _responseData.size();
  HEADER header =
    { HEADERTYPE::SESSION, uncompressedSz, ServerOptions::_compressor, DIAGNOSTICS::NONE, status, 0 };
  utility::compressEncrypt(_buffer, ServerOptions::_encrypted, header, _crypto, _responseData);
  return _responseData;
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
