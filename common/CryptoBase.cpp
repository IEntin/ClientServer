/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoBase.h"

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

void CryptoBase::displayCryptoLibName() {
  std::string_view encryptorLib;
  switch(Options::_encryptorType) {
  case CRYPTO::CRYPTOSODIUM:
    encryptorLib = "Sodium";
    break;
  case CRYPTO::CRYPTOPP:
    encryptorLib = "Crypto++";
    break;
  default:
    encryptorLib = "Error";
    break;
  }
  Info << "\n\nUsing " << encryptorLib << " library.\n\n";
}
