/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cryptopp/secblock.h>

class Crypto {
 public:
  static std::string_view encrypt(const CryptoPP::SecByteBlock& key,
				  std::string_view data);

  static std::string_view decrypt(const CryptoPP::SecByteBlock& key,
				  std::string_view data);

  static void showKey(const CryptoPP::SecByteBlock& key);

  static bool showIv(const CryptoPP::SecByteBlock& iv);
};
