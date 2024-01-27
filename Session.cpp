/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Session.h"

#include <cassert>

#include "DHKeyExchange.h"
#include "PayloadTransform.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"
#include "Utility.h"

Session::Session() :
  _task(std::make_shared<Task>(_response)) {
  _clientId = utility::getUniqueId();
  _Astring = DHKeyExchange::step1(_priv, _pub);
}

void Session::createKey(HEADER& header) {
  auto& [type, payloadSz, uncomprSz, compressor, encrypted, diagnostics, status, parameter] = header;
  assert(payloadSz == _request.size());
  if (type == HEADERTYPE::KEY_EXCHANGE) {
    const char* Bstring = _request.data() + payloadSz - parameter;
    CryptoPP::Integer crossPub(Bstring);
    _key = DHKeyExchange::step2(_priv, crossPub);
    _request.erase(payloadSz - parameter, parameter);
    header =
      { HEADERTYPE::SESSION, payloadSz - parameter, uncomprSz, compressor, encrypted, diagnostics, status, 0 };
  }
}

std::string_view Session::buildReply(HEADER& header, std::atomic<STATUS>& status) {
  if (_response.empty())
    return {};
  header =
    { HEADERTYPE::SESSION, 0, 0, ServerOptions::_compressor, ServerOptions::_encrypted, false, status, 0 };
  _responseData.resize(0);
  for (const auto& entry : _response)
    _responseData.insert(_responseData.end(), entry.begin(), entry.end());
  return payloadtransform::compressEncrypt(_key, _responseData, header);
}

bool Session::processTask(const HEADER& header) {
  auto weakPtr = TaskController::weakInstance();
  if (auto taskController = weakPtr.lock(); taskController) {
    std::string_view restored = payloadtransform::decryptDecompress(_key, header, _request);
    _task->initialize(header, restored);
    taskController->processTask(_task);
    return true;
  }
  return false;
}
