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
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> _obfuscator;
  void hideKey(std::array<unsigned char, crypto_kx_SESSIONKEYBYTES>& key);
  void recoverKey(std::array<unsigned char, crypto_kx_SESSIONKEYBYTES>& key);
  std::atomic<bool> _obfuscated;
};

class CryptoSodium {

  std::string
  hashMessage(std::string_view message);
  std::array<unsigned char, crypto_kx_SECRETKEYBYTES> _secretKeyAes;
  std::array<unsigned char, crypto_kx_PUBLICKEYBYTES> _pubKeyAes;
  std::array<unsigned char, crypto_sign_SECRETKEYBYTES> _secretKeySign;
  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> _publicKeySign;
  std::array<unsigned char, crypto_sign_BYTES> _signature;
  HandleKey _keyHandler; 
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> _key;
  bool checkAccess();
  void setAESKey(std::array<unsigned char, crypto_kx_SESSIONKEYBYTES>& key);
  void eraseUsedData();
  bool _verified = false;
  bool _signatureSent = false;
  std::mutex _mutex;
public:
  explicit CryptoSodium(std::string_view msg);
  CryptoSodium(std::string_view msgHash,
	       std::string_view encodedPubKeyAesClient,
	       std::string_view signatureWithPubKey);
  ~CryptoSodium() = default;
  std::string_view encrypt(std::string& buffer,
			   const HEADER& header,
			   std::string_view data);
  void decrypt(std::string& buffer, std::string& data);
  std::string base64_encode(std::span<unsigned char> input);
  std::vector<unsigned char> base64_decode(std::string_view encoded);
  bool clientKeyExchange(std::string_view encodedPeerPubKeyAes);
  void showKey();
  std::string _msgHash;
  std::string _encodedPubKeyAes;
  std::vector<unsigned char> _signatureWithPubKeySign;

  template <typename L>
  bool sendSignature(L& lambda) {
    HEADER header = { HEADERTYPE::DH_INIT, std::ssize(_msgHash), std::ssize(_encodedPubKeyAes),
		      CRYPTO::NONE, COMPRESSORS::NONE,
		      DIAGNOSTICS::NONE, STATUS::NONE, std::ssize(_signatureWithPubKeySign) };
    bool result = lambda(header, _msgHash, _encodedPubKeyAes, _signatureWithPubKeySign);
    if (result)
      _signatureSent = true;
    eraseUsedData();
    return result;
  }

  CryptoSodium(std::string_view msgHash,
	       std::string_view encodedPubKeyAesClient,
	       std::span<unsigned char> signatureWithPubKey);

}; 
