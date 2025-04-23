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

  std::vector<unsigned char> hashMessage(std::u8string_view message);
  std::vector<unsigned char> encodeLength(size_t length);
  std::vector<unsigned char> _msgHash;
  HandleKey _keyHandler; 
  unsigned char _key[crypto_aead_aes256gcm_KEYBYTES];
  bool _verified = false;
  bool _signatureSent = false;
  std::mutex _mutex;
public:
  explicit CryptoSodium(std::u8string_view msg);
  ~CryptoSodium() = default;
  bool encrypt(std::string& input,
	       const HEADER& header,
	       std::vector<unsigned char>& ciphertext,
	       unsigned long long &ciphertext_len);
  bool decrypt(std::vector<unsigned char> ciphertext,
	       std::string& decrypted);
  void setTestAesKey(unsigned char* key);
  bool checkAccess();
  void setAESKey(unsigned char* key) {
    std::lock_guard lock(_mutex);
    _keyHandler.recoverKey(_key);
    std::copy(_key, _key + crypto_aead_aes256gcm_KEYBYTES, key);
    _keyHandler.hideKey(_key);
  }
  std::string base64_encode(const std::vector<unsigned char>& input);
  std::vector<unsigned char> base64_decode(const std::string& input);
  const std::vector<unsigned char>& getMsgHash() const { return _msgHash; }
};
