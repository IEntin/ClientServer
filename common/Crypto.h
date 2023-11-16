/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cryptopp/secblock.h>

struct CryptoKey {
  CryptoKey() = delete;
  ~CryptoKey() = delete;
  static void showKey();
  static bool initialize();
  static bool keepKey();
  static bool recover();
  static unsigned _cryptoKeySize;
  static CryptoPP::SecByteBlock _key;
  static bool _valid;
};

class Crypto {
 public:
  static std::string_view encrypt(std::string_view data);

  static std::string_view decrypt(std::string_view data);

  static bool showIv(const CryptoPP::SecByteBlock& iv);
};
