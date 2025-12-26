/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoBase.h"

EncryptorVector CryptoBase::_vector;

bool CryptoBase::isEncrypted(std::string_view input) {
  if (input.empty())
    return false;
  assert(input.size() >= HEADER_SIZE);
  HEADER header;
  try {
    if (deserialize(header, &input[0]))
      return false;
    return true;
  }
  catch (const std::runtime_error& error) {
    return true;
  }
}

bool CryptoBase::displayCryptoLibName() {
  std::string_view encryptorLib;
  switch(Options::_encryptorTypeDefault) {
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
  return 0;
}
