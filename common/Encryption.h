/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

struct CryptoKeys {
  CryptoKeys(bool bmaster);
  bool _valid = false;
  std::vector<unsigned char> _key;
  std::vector<unsigned char> _iv;
private:
  bool generate();
  bool recover();
};

class Encryption {
 public:
  static bool encrypt(std::string_view source,
		      const CryptoKeys& cryptoKeys,
		      std::string& cipher);
  static bool decrypt(std::string_view cipher,
		      const CryptoKeys& cryptoKeys,
		      std::string& decrypted);
};
