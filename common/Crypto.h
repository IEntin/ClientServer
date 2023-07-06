/*
 *  Copyright (C) 2021 Ilya Entin
 */


#pragma once

#include <cryptopp/secblock.h>

struct ServerOptions;

struct CryptoKeys {
  CryptoKeys(const ServerOptions& options);
  CryptoKeys();
  void showKeys();
  CryptoPP::SecByteBlock _key;
  CryptoPP::SecByteBlock _iv;
  bool _valid = false;
private:
  bool generate();
  bool recover();
};

class Crypto {
 public:
  static void encrypt(std::string& in_out, const CryptoKeys& keys);

  static void decrypt(std::string_view cipher,
		      const CryptoKeys& keys,
		      std::string& decrypted);
};
