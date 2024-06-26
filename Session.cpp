/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Session.h"

#include <cassert>
#include <cryptopp/aes.h>

#include <boost/algorithm/hex.hpp>

#include "Crypto.h"
#include "Logger.h"
#include "PayloadTransform.h"
#include "Server.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"
#include "Utility.h"

Session::Session(ServerWeakPtr server, std::string_view pubBstring) :
  _dhA(Crypto::_curve),
  _privA(_dhA.PrivateKeyLength()),
  _pubA(_dhA.PublicKeyLength()),
  _generatedKeyPair(Crypto::generateKeyPair(_dhA, _privA, _pubA)),
  _pubAstrng(reinterpret_cast<const char*>(_pubA.data()), _pubA.size()),
  _sharedA(_dhA.AgreedValueLength()),
  _task(std::make_shared<Task>(_response)),
  _server(server) {
  CryptoPP::SecByteBlock pubBreceived(reinterpret_cast<const byte*>(pubBstring.data()), pubBstring.size());
  bool rtnA = _dhA.Agree(_sharedA, _privA, pubBreceived);
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
  header =
    { HEADERTYPE::SESSION, 0, 0, ServerOptions::_compressor, ServerOptions::_encrypted, false, status, 0 };
  _responseData.clear();
  for (const auto& entry : _response)
    _responseData.insert(_responseData.end(), entry.begin(), entry.end());
  return payloadtransform::compressEncrypt(_sharedA, _responseData, header);
}

bool Session::processTask(const HEADER& header) {
  auto weakPtr = TaskController::weakInstance();
  if (auto taskController = weakPtr.lock(); taskController) {
    std::string_view restored = payloadtransform::decryptDecompress(_sharedA, header, _request);
    _task->update(isDiagnosticsEnabled(header), restored);
    taskController->processTask(_task);
    return true;
  }
  return false;
}
