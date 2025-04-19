/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>
#include <vector>

#include <sodium.h>

#include "Header.h"

struct HandleKey {
  explicit HandleKey();
  ~HandleKey() = default;
  std::size_t _size;
  std::vector<unsigned char> _obfuscator;
  void hideKey(std::vector<unsigned char>& key);
  void recoverKey(std::vector<unsigned char>& key);
  std::atomic<bool> _obfuscated;
};

class CryptoSodium {

  std::vector<unsigned char> encodeLength(size_t length);
  HandleKey _keyHandler; 
public:
  CryptoSodium();
  ~CryptoSodium() = default;
  std::string_view encrypt(std::string& buffer, const HEADER& header, std::string_view data);
  void decrypt(std::string& buffer, std::string& data);
  std::vector<unsigned char> berEncode(const std::string& str);
  std::string berDecode(const std::vector<unsigned char>& encoded);
  std::string base64Encode(const std::vector<unsigned char>& input);
  std::vector<unsigned char> base64Decode(const std::string& input);
};
