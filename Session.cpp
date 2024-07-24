/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Session.h"

#include <cryptopp/aes.h>

#include "Crypto.h"
#include "Logger.h"
#include "PayloadTransform.h"
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

std::string_view Session::buildReply(HEADER& header, std::atomic<STATUS>& status) {
  if (_response.empty())
    return {};
  _responseData.clear();
  for (const auto& entry : _response)
    _responseData.insert(_responseData.end(), entry.begin(), entry.end());
  header =
    { HEADERTYPE::SESSION, 0, _responseData.size(), ServerOptions::_compressor, ServerOptions::_encrypted, DIAGNOSTICS::NONE, status, 0 };
  return payloadtransform::compressEncrypt(_sharedA, _responseData, header);
}

bool Session::processTask(const HEADER& header) {
  auto weakPtr = TaskController::getWeakPtr();
  if (auto taskController = weakPtr.lock(); taskController) {
    std::string_view restored = payloadtransform::decryptDecompress(_sharedA, header, _request);
    _task->update(isDiagnosticsEnabled(header), restored);
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
