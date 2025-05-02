/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <array>
#include <atomic>
#include <vector>

#include <sodium.h>

#include "Header.h"

using CryptoSodiumPtr = std::shared_ptr<class CryptoSodium>;
using CryptoWeakSodiumPtr = std::weak_ptr<class CryptoSodium>;

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

  std::array<unsigned char, crypto_generichash_BYTES>
  hashMessage(std::u8string_view message);
  std::vector<unsigned char> encodeLength(size_t length);
  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> _publicKeySign;
  unsigned char _secretKeySign[crypto_sign_SECRETKEYBYTES];
  const std::array<unsigned char, crypto_generichash_BYTES> _msgHash;
  std::array<unsigned char, crypto_sign_BYTES> _signature;
  HandleKey _keyHandler; 
  unsigned char _key[crypto_aead_aes256gcm_KEYBYTES];
  void showKey();
  bool checkAccess();
  void setAESKey(unsigned char* key) {
    std::lock_guard lock(_mutex);
    _keyHandler.recoverKey(_key);
    std::copy(_key, _key + crypto_aead_aes256gcm_KEYBYTES, key);
    _keyHandler.hideKey(_key);
  }
  bool _verified = false;
  bool _signatureSent = false;
  std::mutex _mutex;
public:
  explicit CryptoSodium(std::u8string_view msg);
  ~CryptoSodium() = default;
  std::string_view encrypt(std::string& buffer,
			   const HEADER& header,
			   std::string_view data);
  void decrypt(std::string& buffer, std::string& data);
  void setDummyAesKey();
  std::string base64_encode(const std::vector<unsigned char>& input);
  std::vector<unsigned char> base64_decode(const std::string& input);
  const std::array<unsigned char, crypto_generichash_BYTES>&
  getMsgHash() const { return _msgHash; }

  const std::array<unsigned char, crypto_sign_PUBLICKEYBYTES>&
  getPublicKeySign() const { return _publicKeySign; }

  const std::array<unsigned char, crypto_sign_BYTES>&
  getSignature() const { return _signature; }
};
