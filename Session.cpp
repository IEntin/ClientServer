/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Session.h"

#include <cassert>

#include "DHKeyExchange.h"
#include "Task.h"
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
    std::string Bstring = _request.substr(payloadSz - parameter, parameter);
    CryptoPP::Integer crossPub(Bstring.c_str());
    _key = DHKeyExchange::step2(_priv, crossPub);
    _request.erase(payloadSz - parameter, parameter);
    header =
      { HEADERTYPE::SESSION, payloadSz - parameter, uncomprSz, compressor, encrypted, diagnostics, status, 0 };
  }
}
