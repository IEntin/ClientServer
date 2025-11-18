/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <array>
#include <atomic>

#include <sodium.h>

#include "CryptoBase.h"

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

class CryptoSodium : public CryptoBase {
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
  void eraseUsedData();
  const static std::string _name;
public:
  explicit CryptoSodium();
  CryptoSodium(std::string_view encodedPeerAesPubKey,
	       std::string_view signatureWithPubKey);
  ~CryptoSodium() override;
  const std::string&  getName() const override { return _name; }
  std::string_view encrypt(std::string& buffer,
			   const HEADER& header,
			   std::string_view data);
  void decrypt(std::string& buffer, std::string& data);
  std::string base64_encode(std::span<unsigned char> input);
  std::vector<unsigned char> base64_decode(std::string_view encoded);
  bool clientKeyExchange(std::string_view encodedPeerPubKeyAes);
  std::string _msgHash;
  std::string _encodedPubKeyAes;
  std::string _signatureWithPubKeySign;
  template <typename L>
  bool sendSignature(L& lambda) {
    HEADER header = { HEADERTYPE::DH_INIT, 0, _encodedPubKeyAes.size(),
		      COMPRESSORS::NONE,
		      DIAGNOSTICS::NONE, STATUS::NONE, _signatureWithPubKeySign.size() };
    bool result = lambda(header, _encodedPubKeyAes, _signatureWithPubKeySign);
    if (result)
      _signatureSent = true;
    eraseUsedData();
    return result;
  }

}; 
