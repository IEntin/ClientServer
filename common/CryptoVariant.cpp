
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoVariant.h"
#include "EncryptorTemplates.h"

namespace cryptovariant {

bool _initialized = false;

CryptoVariant _clientEncryptorVariant0;
CryptoVariant _serverEncryptorVariant0;
CryptoVariant _clientEncryptorVariant1;
CryptoVariant _serverEncryptorVariant1;

bool isInitialized(){
  return _initialized;
}

bool initialize() {
  CryptoSodiumPtr clientEncryptor0 = std::make_shared<CryptoSodium>();
  CryptoSodiumPtr serverEncryptor0 = encryptortemplates::createServerEncryptor(clientEncryptor0);
  clientEncryptor0->clientKeyExchange(serverEncryptor0->_encodedPubKeyAes);
  CryptoPlPlPtr clientEncryptor1 = std::make_shared<CryptoPlPl>();
  CryptoPlPlPtr serverEncryptor1 = encryptortemplates::createServerEncryptor(clientEncryptor1);
  clientEncryptor1->clientKeyExchange(serverEncryptor1->_encodedPubKeyAes);
  _clientEncryptorVariant0 = std::move(clientEncryptor0);
  _serverEncryptorVariant0 = std::move(serverEncryptor0);
  _clientEncryptorVariant1 = std::move(clientEncryptor1);
  _serverEncryptorVariant1 = std::move(serverEncryptor1);
  _initialized = true;
  return true;
}

const CryptoVariant& getClientEncryptorVariant(CRYPTO crypto) {
  if (!isInitialized())
    initialize();
  switch(crypto) {
  case CRYPTO::CRYPTOSODIUM:
    return _clientEncryptorVariant0;
  case CRYPTO::CRYPTOPP:
    return _clientEncryptorVariant1;
  default:
    throw std::runtime_error("Bad variant index");
  }
}

const CryptoVariant& getServerEncryptorVariant(CRYPTO crypto) {
  if (!isInitialized())
    initialize();
  switch(crypto) {
  case CRYPTO::CRYPTOSODIUM:
    return _serverEncryptorVariant0;
  case CRYPTO::CRYPTOPP:
    return _serverEncryptorVariant1;
  default:
    throw std::runtime_error("Bad variant index");
  }
}

} // end of namespace cryptovariant
