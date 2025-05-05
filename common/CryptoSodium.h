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

  std::vector<unsigned char>
  hashMessage(std::u8string_view message);
  std::vector<unsigned char> encodeLength(size_t length);
  unsigned char _secretKey[crypto_kx_SECRETKEYBYTES];
  std::array<unsigned char, crypto_kx_PUBLICKEYBYTES> _publicKey;
  unsigned char _secretKeySign[crypto_sign_SECRETKEYBYTES];
  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> _publicKeySign;
  const std::vector<unsigned char> _msgHash;
  std::array<unsigned char, crypto_sign_BYTES> _signature;
  std::vector<unsigned char> _signatureWithPubKeySign;
  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> _peerSignPubKey;
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
  CryptoSodium(std::span<const unsigned char> msgHash,
	       std::span<const unsigned char> pubB,
	       std::span<const unsigned char> signatureWithPubKey);
  ~CryptoSodium() = default;
  std::string_view encrypt(std::string& buffer,
			   const HEADER& header,
			   std::string_view data);
  void decrypt(std::string& buffer, std::string& data);
  void setDummyAesKey();
  std::string base64_encode(std::span<const unsigned char> input);
  std::vector<unsigned char> base64_decode(const std::string& input);
  // used in tests:
  std::span<const unsigned char>
  getSignatureWithPubKeySign() const { return _signatureWithPubKeySign; }
  // used in tests:
  std::span<const unsigned char> getMsgHash() const { return _msgHash; }
  // used in tests:
  std::span<const unsigned char>
  getPublicKey() const { return _publicKey; }

  std::span<const unsigned char>
  getPublicKeySign() const { return _publicKeySign; }
  // used in tests:
  std::span<const unsigned char>
  getSignature() const { return _signature; }
  bool isVerified() const { return _verified; }
};
