
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"
#include "EncryptorTemplates.h"

CryptoTuple _clientEncryptorTuple = { std::make_shared<CryptoSodium>(), std::make_shared<CryptoPlPl>() };

CryptoTuple initServerEncryptorTuple() {
  CryptoSodiumPtr clientEncryptor0 = std::get<0>(_clientEncryptorTuple);
  CryptoSodiumPtr serverEncryptor0 = createServerEncryptor(clientEncryptor0);
  clientEncryptor0->clientKeyExchange(serverEncryptor0->_encodedPubKeyAes);
  CryptoPlPlPtr clientEncryptor1 = std::get<1>(_clientEncryptorTuple);
  CryptoPlPlPtr serverEncryptor1 = createServerEncryptor(clientEncryptor1);
  clientEncryptor1->clientKeyExchange(serverEncryptor1->_encodedPubKeyAes);
  return { serverEncryptor0, serverEncryptor1 };
}

CryptoTuple _serverEncryptorTuple = initServerEncryptorTuple();
