/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <array>
#include <atomic>

#include <sodium.h>

#include "CryptoBase.h"
#include "IOUtility.h"

using CryptoSodiumPtr = std::shared_ptr<class CryptoSodium>;
using CryptoWeakSodiumPtr = std::weak_ptr<class CryptoSodium>;

struct HandleKey {
  explicit HandleKey();
  ~HandleKey();
  std::size_t _size;
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> _obfuscator;
  void hideKey(std::array<unsigned char, crypto_kx_SESSIONKEYBYTES>& key);
  void recoverKey(std::array<unsigned char, crypto_kx_SESSIONKEYBYTES>& key);
  std::atomic<bool> _obfuscated;
};

class CryptoSodium : public CryptoBase, public std::enable_shared_from_this<CryptoSodium> {
  friend struct RemoveSensitiveData;
  using SessionKey = std::array<unsigned char, crypto_kx_SESSIONKEYBYTES>;
  std::string
  hashMessage(std::string_view message);
  std::array<unsigned char, crypto_kx_SECRETKEYBYTES> _privKeyAes;
  std::array<unsigned char, crypto_kx_PUBLICKEYBYTES> _pubKeyAes;
  std::array<unsigned char, crypto_kx_PUBLICKEYBYTES> _peerPubKeyAes;
  std::array<unsigned char, crypto_sign_SECRETKEYBYTES> _secretKeySign;
  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> _publicKeySign;
  std::array<unsigned char, crypto_sign_BYTES> _signature;
  HandleKey _keyHandler; 
  SessionKey _key;
  bool checkAccess();
  void hideKey();
  void setAESKey(SessionKey& key);
  struct RemoveSensitiveData {
    explicit RemoveSensitiveData(CryptoSodium* ptr) : _ptr(ptr) {}
    ~RemoveSensitiveData() {
      sodium_memzero(_ptr->_privKeyAes.data(), _ptr->_privKeyAes.size());
      sodium_memzero(_ptr->_msgHash.data(), _ptr->_msgHash.size());
      sodium_memzero(_ptr->_signatureWithPubKeySign.data(), _ptr->_signatureWithPubKeySign.size());
    }
    CryptoSodium* _ptr = nullptr;
  };
public:
  explicit CryptoSodium();
  CryptoSodium(std::string_view encodedPeerAesPubKey,
	       std::string_view signatureWithPubKey);
  ~CryptoSodium() override;
  std::string_view  getName() const override { return "CryptoSodium"; }
  std::string_view encrypt(std::string& buffer,
			   const HEADER* const header,
			   std::string_view data);
  void decrypt(std::string& buffer, std::string& data);
  std::string base64_encode(std::span<unsigned char> input);
  std::vector<unsigned char> base64_decode(std::string_view encoded);
  bool clientKeyExchange(std::string_view encodedPeerPubKeyAes);
  std::string _msgHash;
  std::string _encodedPubKeyAes;
  std::string _signatureWithPubKeySign;
  
  bool sendSignature() {
    if (!_signatureSent) {
      _signatureSent = true;
    }
    sodium_memzero(_signatureWithPubKeySign.data(), _signatureWithPubKeySign.size());
    return _signatureSent;
  }

}; 
