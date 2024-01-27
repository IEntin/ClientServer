/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cryptopp/secblock.h>

class Crypto {
  Crypto() = delete;
  ~Crypto() = delete;
 public:
  static std::string_view encrypt(const CryptoPP::SecByteBlock& key,
				  std::string_view data);

  static std::string_view decrypt(const CryptoPP::SecByteBlock& key,
				  std::string_view data);

  static void showKeyIv(const CryptoPP::SecByteBlock& key,
			const CryptoPP::SecByteBlock& iv);
};
