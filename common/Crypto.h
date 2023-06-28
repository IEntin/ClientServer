/*
 *  Copyright (C) 2021 Ilya Entin
 */


#pragma once

#include <cryptopp/modes.h>
#include <cryptopp/secblock.h>

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
  static void encrypt(std::string_view source,
		      const CryptoKeys& keys,
		      std::string& cipher);
  static void decrypt(std::string_view cipher,
		      const CryptoKeys& keys,
		      std::string& decrypted);
};
