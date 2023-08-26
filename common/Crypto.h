/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cryptopp/secblock.h>

struct ServerOptions;

struct CryptoKey {
  CryptoKey(const ServerOptions& options);
  CryptoKey();
  ~CryptoKey() = default;
  void showKey() const;
  CryptoPP::SecByteBlock _key;
  bool _valid = false;
private:
  bool generate();
  bool recover();
};

class Crypto {
 public:
  static void encrypt(std::string& data);

  static void decrypt(std::string& cipher);

  static void showIv(const CryptoPP::SecByteBlock& iv);
};
