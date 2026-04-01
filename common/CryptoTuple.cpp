
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"

namespace cryptotuple {

bool _initialized = false;

CryptoTuple _clientEncryptorTuple;
CryptoTuple _serverEncryptorTuple;

bool isInitialized(){
  return _initialized;
}

bool initialize() {
  static CryptoSodiumPtr clientEncryptor0 = std::make_shared<CryptoSodium>();
  static CryptoSodiumPtr serverEncryptor0 = createServerEncryptor(clientEncryptor0);
  clientEncryptor0->clientKeyExchange(serverEncryptor0->_encodedPubKeyAes);
  static CryptoPlPlPtr clientEncryptor1 = std::make_shared<CryptoPlPl>();
  static CryptoPlPlPtr serverEncryptor1 = createServerEncryptor(clientEncryptor1);
  clientEncryptor1->clientKeyExchange(serverEncryptor1->_encodedPubKeyAes);

  _clientEncryptorTuple = std::make_tuple(clientEncryptor0, clientEncryptor1);
  _serverEncryptorTuple = std::make_tuple(serverEncryptor0, serverEncryptor1);
  _initialized = true;
  return true;
}

CryptoTuple getClientEncryptorTuple() {
  if (!isInitialized())
    initialize();
  return _clientEncryptorTuple;
}

CryptoTuple getServerEncryptorTuple() {
  if (!isInitialized())
    initialize();
  return _serverEncryptorTuple;
}

} // end of namespace cryptotuple
