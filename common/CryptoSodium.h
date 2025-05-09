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
  std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES> _obfuscator;
  void hideKey(std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES>& key);
  void recoverKey(std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES>& key);
  std::atomic<bool> _obfuscated;
};

class CryptoSodium {

  std::vector<unsigned char>
  hashMessage(std::u8string_view message);
  std::vector<unsigned char> encodeLength(size_t length);
  std::array<unsigned char, crypto_kx_SECRETKEYBYTES> _secretKeyAes;
  std::array<unsigned char, crypto_kx_PUBLICKEYBYTES> _publicKeyAes;
  std::array<unsigned char, crypto_sign_SECRETKEYBYTES> _secretKeySign;
  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> _publicKeySign;
  std::vector<unsigned char> _msgHash;
  std::array<unsigned char, crypto_sign_BYTES> _signature;
  std::vector<unsigned char> _signatureWithPubKeySign;
  HandleKey _keyHandler; 
  std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES> _key;
  void showKey();
  bool checkAccess();
  void setAESKey(std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES>& key) {
    std::lock_guard lock(_mutex);
    _keyHandler.recoverKey(_key);
    key = _key;
    _keyHandler.hideKey(_key);
  }
  void eraseUsedData();
  bool _verified = false;
  bool _signatureSent = false;
  std::mutex _mutex;
public:
  explicit CryptoSodium(std::u8string_view msg);
  CryptoSodium(std::span<const unsigned char> msgHash,
	       std::span<const unsigned char> pubKeyAesClient,
	       std::span<const unsigned char> signatureWithPubKey);
  ~CryptoSodium() = default;
  std::string_view encrypt(std::string& buffer,
			   const HEADER& header,
			   std::string_view data);
  void decrypt(std::string& buffer, std::string& data);
  void setDummyAesKey();
  std::string base64_encode(std::span<const unsigned char> input);
  std::vector<unsigned char> base64_decode(const std::string& input);
  bool clientKeyExchange(std::span<const unsigned char> peerPublicKeyAes);
  // used in tests:
  std::span<const unsigned char>
  getSignatureWithPubKeySign() const { return _signatureWithPubKeySign; }
  // used in tests:
  std::span<const unsigned char> getMsgHash() const { return _msgHash; }
  // used in tests:
  std::span<const unsigned char>
  getPublicKeyAes() const { return _publicKeyAes; }

  std::span<const unsigned char>
  getPublicKeySign() const { return _publicKeySign; }
  // used in tests:
  std::span<const unsigned char>
  getSignature() const { return _signature; }
  bool isVerified() const { return _verified; }
  // used in tests:
  std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES> getAesKey();
  
}; 
