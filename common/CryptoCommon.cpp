/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoCommon.h"

namespace cryptocommon {

ENCRYPTORCONTAINER _encryptorContainer;

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

bool displayCryptoLibName() {
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

}// end of namespace cryptocommon
