/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Session.h"

#include <cassert>

#include "DHKeyExchange.h"
#include "Logger.h"
#include "PayloadTransform.h"
#include "Server.h"
#include "ServerOptions.h"
#include "Task.h"
#include "TaskController.h"
#include "Utility.h"

Session::Session(ServerWeakPtr server, std::string_view Bstring) :
  _task(std::make_shared<Task>(_response)),
  _server(server) {
  _clientId = utility::getUniqueId();
  _Astring = DHKeyExchange::step1(_priv, _pub);
  CryptoPP::Integer crossPub(Bstring.data());
  assert(_pub != crossPub);
  _key = DHKeyExchange::step2(_priv, crossPub);
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
