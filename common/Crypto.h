/*
 *  Copyright (C) 2021 Ilya Entin
 */


#pragma once

#include <cryptopp/secblock.h>

struct ServerOptions;

struct CryptoKeys {
  CryptoKeys(const ServerOptions& options);
  CryptoKeys();
  ~CryptoKeys() = default;
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
  static void encrypt(std::string& data, const CryptoKeys& keys);

  static void decrypt(std::string& cipher, const CryptoKeys& keys);
};
