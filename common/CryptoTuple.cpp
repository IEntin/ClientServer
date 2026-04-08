
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

std::string doubleEncrypt(const CryptoTuple& tuple,
			  std::string& buffer,
			  const HEADER& header,
			  std::string& source) {
  std::string encrypted;
  CryptoWeakSodiumPtr cryptoWeakSodiumPtr = std::get<CryptoWeakSodiumPtr>(tuple);
  if (CryptoSodiumPtr encryptorSodium = cryptoWeakSodiumPtr.lock()) {
    buffer.clear();
    encrypted = encryptorSodium->encrypt(buffer, &header, source);
  }
  CryptoWeakPlPlPtr cryptoWeakPlPlPtr = std::get<CryptoWeakPlPlPtr>(tuple);
  if (CryptoPlPlPtr encryptorPlPl = cryptoWeakPlPlPtr.lock()) {
    buffer.clear();
    encrypted = encryptorPlPl->encrypt(buffer, nullptr, encrypted);
  }
  return encrypted;
}

std::string doubleDecrypt(const CryptoTuple& tuple,
			  std::string& buffer,
			  HEADER& header,
			  std::string& data) {
  std::string decrypted;
  CryptoWeakPlPlPtr cryptoWeakPlPlPtr = std::get<CryptoWeakPlPlPtr>(tuple);
  if (CryptoPlPlPtr encryptorPlPlPtr = cryptoWeakPlPlPtr.lock()) {
    buffer.clear();
    encryptorPlPlPtr->decrypt(buffer, data);
    decrypted = buffer;
  }
  CryptoWeakSodiumPtr cryptoWeakSodiumPtr = std::get<CryptoWeakSodiumPtr>(tuple);
  if (CryptoSodiumPtr encryptorSodiumPtr = cryptoWeakSodiumPtr.lock()) {
    buffer.clear();
    encryptorSodiumPtr->decrypt(buffer, decrypted);
    decrypted = buffer;
  }
  if (!deserialize(header, decrypted.data()))
    throw std::runtime_error("doubleDecrypt failure.");
  return decrypted;
}

} // end of namespace cryptotuple
