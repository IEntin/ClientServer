/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cryptopp/secblock.h>

struct ServerOptions;

struct CryptoKey {
  CryptoKey() = delete;
  ~CryptoKey() = delete;
  static void showKey();
  static bool initialize(const ServerOptions& options);
  static bool recover();
  static CryptoPP::SecByteBlock _key;
  static bool _valid;
};

class Crypto {
 public:
  static void encrypt(std::string& data);

  static void decrypt(std::string& cipher);

  static bool showIv(const CryptoPP::SecByteBlock& iv);
};
