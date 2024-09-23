/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Session.h"

#include <cryptopp/aes.h>

#include "Crypto.h"
#include "Logger.h"
#include "Server.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"
#include "Utility.h"

Session::Session(ServerWeakPtr server, const CryptoPP::SecByteBlock& pubB) :
  _dhA(Crypto::_curve),
  _privA(_dhA.PrivateKeyLength()),
  _pubA(_dhA.PublicKeyLength()),
  _generatedKeyPair(Crypto::generateKeyPair(_dhA, _privA, _pubA)),
  _sharedA(_dhA.AgreedValueLength()),
  _task(std::make_shared<Task>(_response)),
  _server(server) {
  bool rtnA = _dhA.Agree(_sharedA, _privA, pubB);
  if(!rtnA)
    throw std::runtime_error("Failed to reach shared secret (A)");
  _clientId = utility::getUniqueId();
}

Session::~Session() {
  Trace << '\n';
}

std::string_view Session::buildReply(std::atomic<STATUS>& status) {
  if (_response.empty())
    return {};
  _responseData.erase(0);
  for (const auto& entry : _response)
    _responseData.insert(_responseData.end(), entry.begin(), entry.end());
  std::size_t uncompressedSz = _responseData.size();
  HEADER header =
    { HEADERTYPE::SESSION, uncompressedSz, ServerOptions::_compressor, DIAGNOSTICS::NONE, status, 0 };
  return utility::compressEncrypt(ServerOptions::_encrypted, header, _sharedA, _responseData);
}

bool Session::processTask() {
  std::string_view restored = utility::decryptDecompress(_task->header(), _sharedA, _request);
  auto weakPtr = TaskController::getWeakPtr();
  if (auto taskController = weakPtr.lock(); taskController) {
    _task->update(restored);
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
