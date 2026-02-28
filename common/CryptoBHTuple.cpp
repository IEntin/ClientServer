
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoBHTuple.h"
#include "EncryptorTemplates.h"

namespace cryptobhtuple {

CRYPTOBHTUPLE _clientEncryptors;
CRYPTOBHTUPLE _serverEncryptors;

bool initialize() {
  CryptoSodiumPtr clientEncryptor0 = std::make_shared<CryptoSodium>();
  CryptoSodiumPtr serverEncryptor0 = createServerEncryptor(clientEncryptor0);
  clientEncryptor0->clientKeyExchange(serverEncryptor0->_encodedPubKeyAes);
  
  CryptoPlPlPtr clientEncryptor1 = std::make_shared<CryptoPlPl>();
  CryptoPlPlPtr serverEncryptor1 = createServerEncryptor(clientEncryptor1);
  clientEncryptor1->clientKeyExchange(serverEncryptor1->_encodedPubKeyAes);

  _clientEncryptors = boost::hana::make_tuple(clientEncryptor0, clientEncryptor1);
  _serverEncryptors = boost::hana::make_tuple(serverEncryptor0, serverEncryptor1);
  return true;
}

CRYPTOBHTUPLE getClientEncryptors() {
  return _clientEncryptors;
}

CRYPTOBHTUPLE getServerEncryptors() {
  return _serverEncryptors;
}

} // end of namespace cryptotuple
