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
  unsigned char _obfuscator[crypto_aead_aes256gcm_KEYBYTES];
  void hideKey(unsigned char* key);
  void recoverKey(unsigned char* key);
  std::atomic<bool> _obfuscated;
};

class CryptoSodium {

  std::vector<unsigned char> encodeLength(size_t length);
  HandleKey _keyHandler; 
  unsigned char _key[crypto_aead_aes256gcm_KEYBYTES];
  std::mutex _mutex;
public:
  CryptoSodium();
  ~CryptoSodium() = default;
  bool encrypt(std::string& input,
	       const HEADER& header,
	       std::vector<unsigned char>& ciphertext,
	       unsigned long long &ciphertext_len);
  bool decrypt(std::vector<unsigned char> ciphertext,
	       std::string& decrypted);
  std::vector<unsigned char> berEncode(const std::string& str);
  std::string berDecode(const std::vector<unsigned char>& encoded);
  std::string base64Encode(const std::vector<unsigned char>& input);
  std::vector<unsigned char> base64Decode(const std::string& input);
  void setTestAesKey(unsigned char* key);
  bool checkAccess();
  void setAESKey(unsigned char* key) {
    std::lock_guard lock(_mutex);
    _keyHandler.recoverKey(_key);
    std::copy(_key, _key + crypto_aead_aes256gcm_KEYBYTES, key);
    _keyHandler.hideKey(_key);
  }
};
