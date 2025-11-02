/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <sodium.h>

#include "CryptoCommon.h"

namespace cryptocommon {

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
  switch(cryptocommon::_encryptorDefault) {
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

}// end of namespace cryptocommon
