
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoVariant.h"
#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"

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
  CryptoSodiumPtr serverEncryptor0 = createServerEncryptor(clientEncryptor0);
  clientEncryptor0->clientKeyExchange(serverEncryptor0->_encodedPubKeyAes);
  CryptoPlPlPtr clientEncryptor1 = std::make_shared<CryptoPlPl>();
  CryptoPlPlPtr serverEncryptor1 = createServerEncryptor(clientEncryptor1);
  clientEncryptor1->clientKeyExchange(serverEncryptor1->_encodedPubKeyAes);
  _clientEncryptorVariant0 = std::move(clientEncryptor0);
  _serverEncryptorVariant0 = std::move(serverEncryptor0);
  _clientEncryptorVariant1 = std::move(clientEncryptor1);
  _serverEncryptorVariant1 = std::move(serverEncryptor1);
  _initialized = true;
  return true;
}

CryptoVariant& getEncryptorVariant(APPTYPE app, CRYPTO crypto) {
  static CryptoVariant emptyVariant;
  if (!isInitialized())
    initialize();
  switch (app) {
  case APPTYPE::SERVER:
    switch (crypto) {
    case CRYPTO::CRYPTOSODIUM:
      return _serverEncryptorVariant0;
    case CRYPTO::CRYPTOPP:
      return _serverEncryptorVariant1;
    default:
      return emptyVariant;
    };
    case APPTYPE::CLIENT:
    switch (crypto) {
    case CRYPTO::CRYPTOSODIUM:
      return _clientEncryptorVariant0;
    case CRYPTO::CRYPTOPP:
      return _clientEncryptorVariant1;
    default:
      return emptyVariant;
    };
    case APPTYPE::TESTS:
      return emptyVariant;
  };
  return emptyVariant;
}

} // end of namespace cryptovariant
