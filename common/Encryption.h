/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

struct CryptoKeys {
  CryptoKeys();
  std::vector<unsigned char> _key;
  std::vector<unsigned char> _iv;
  bool _valid = false;
private:
  bool recover();
};

class Encryption {
 public:
  static void initialize();
  static bool encrypt(std::string_view source,
		      const std::vector<unsigned char>& key,
		      const std::vector<unsigned char>& iv,
		      std::string& cipher);
  static bool decrypt(std::string_view cipher,
		      const std::vector<unsigned char>& key,
		      const std::vector<unsigned char>& iv,
		      std::string& decrypted);
  static const std::vector<unsigned char>& getKey() { return _key; }
  static const std::vector<unsigned char>& getIv() { return _iv; }
 private:
  static std::vector<unsigned char> _key;
  static std::vector<unsigned char> _iv;
};
