/*
 *  Copyright (C) 2021 Ilya Entin
 */


#pragma once

#include "modes.h"
#include "secblock.h"

struct CryptoKeys {
  CryptoKeys(bool bmaster, bool invalidateKeys = true);
  bool _valid = false;
  CryptoPP::SecByteBlock _key;
  CryptoPP::SecByteBlock _iv;
private:
  bool generate();
  bool recover();
};

class Crypto {
 public:
  static bool encrypt(std::string_view source,
		      const CryptoKeys& keys,
		      std::string& cipher);
  static bool decrypt(std::string_view cipher,
		      const CryptoKeys& keys,
		      std::string& decrypted);
};
