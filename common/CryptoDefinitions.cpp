/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoDefinitions.h"

namespace cryptodefinitions {

bool initialize() {
  std::string_view encryptorLib;
  switch(_encryptorDefault) {
  case CRYPTO::CRYPTOPP:
    encryptorLib = "Crypto++";
    break;
  case CRYPTO::CRYPTOSODIUM:
    encryptorLib = "Sodium";
    break;
  default:
    encryptorLib = "Error";
    break;
  }
  Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
  logger << "\nUsing " << encryptorLib << " library.\n\n";
  int initialized = sodium_init();
  assert(initialized == 0 && "libsodium initialization failed");
  return 0;
}

bool isEncrypted(std::string_view input) {
  if (input.empty())
    return false;
  assert(input.size() >= HEADER_SIZE);
  HEADER header;
  std::string inputStr(input.cbegin(), input.cbegin() + HEADER_SIZE);
  try {
    if (deserialize(header, inputStr.data()))
      return false;
    return true;
  }
  catch (const std::runtime_error& error) {
    return true;
  }
}

std::any getEncryptor(std::tuple<CryptoPlPlPtr, CryptoSodiumPtr> tuple,
		      std::optional<CRYPTO> encryptor) {
  CRYPTO encryptionVal = encryptor.has_value() ? *encryptor : _encryptorDefault;  
  switch (encryptionVal) {
  case CRYPTO::CRYPTOPP:
    return std::get<CryptoPlPlPtr>(tuple);
  case CRYPTO::CRYPTOSODIUM:
    return std::get<CryptoSodiumPtr>(tuple);
  default:
    return nullptr;
  }
}

std::any getEncryptor(std::variant<CryptoPlPlPtr, CryptoSodiumPtr> var,
		      std::optional<CRYPTO> encryptor) {
  CRYPTO encryptionVal = encryptor.has_value() ? *encryptor : _encryptorDefault;  
  switch (encryptionVal) {
  case CRYPTO::CRYPTOPP:
    return std::get<CryptoPlPlPtr>(var);
  case CRYPTO::CRYPTOSODIUM:
    return std::get<CryptoSodiumPtr>(var);
  default:
    return nullptr;
  }
}

} // end of namespace cryptodefinitions
