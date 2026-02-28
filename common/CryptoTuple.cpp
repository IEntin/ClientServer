
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"
#include "EncryptorTemplates.h"

namespace cryptotuple {

bool _initialized = false;

CryptoTuple _clientEncryptorTuple;
CryptoTuple _serverEncryptorTuple;

bool isInitialized(){
  return _initialized;
}

bool initialize() {
  CryptoSodiumPtr clientEncryptor0 = std::make_shared<CryptoSodium>();
  CryptoSodiumPtr serverEncryptor0 = createServerEncryptor(clientEncryptor0);
  clientEncryptor0->clientKeyExchange(serverEncryptor0->_encodedPubKeyAes);
  CryptoPlPlPtr clientEncryptor1 = std::make_shared<CryptoPlPl>();
  CryptoPlPlPtr serverEncryptor1 = createServerEncryptor(clientEncryptor1);
  clientEncryptor1->clientKeyExchange(serverEncryptor1->_encodedPubKeyAes);

  _clientEncryptorTuple = std::make_tuple(clientEncryptor0, clientEncryptor1);
  _serverEncryptorTuple = std::make_tuple(serverEncryptor0, serverEncryptor1);
  _initialized = true;;
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
