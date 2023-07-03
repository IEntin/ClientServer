/*
 *  Copyright (C) 2021 Ilya Entin
 */


#pragma once

#include <cryptopp/secblock.h>

struct Options;

struct CryptoKeys {
  CryptoKeys(const Options& options);
  CryptoKeys();
  CryptoPP::SecByteBlock _key;
  CryptoPP::SecByteBlock _iv;
  bool _valid = false;
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
