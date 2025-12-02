/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoBase.h"

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
