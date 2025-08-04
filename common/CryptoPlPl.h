/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cryptopp/aes.h>
#include <cryptopp/asn.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/modes.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/rsa.h>
#include <cryptopp/secblock.h>

#include "Header.h"

constexpr std::size_t RSA_KEY_SIZE = 2048;

using CryptoPlPlPtr = std::shared_ptr<class CryptoPlPl>;
using CryptoWeakPlPlPtr = std::weak_ptr<class CryptoPlPl>;

struct KeyHandler {
  explicit KeyHandler(unsigned size);
  ~KeyHandler() = default;
  const unsigned _size;
  CryptoPP::AutoSeededX917RNG<CryptoPP::AES> _rng;
  CryptoPP::SecByteBlock _obfuscator;
  void hideKey(CryptoPP::SecByteBlock& key);
  void recoverKey(CryptoPP::SecByteBlock& key);
  std::atomic<bool> _obfuscated = false;
};

// version using Crypto++ library
class CryptoPlPl {
  CryptoPP::AutoSeededX917RNG<CryptoPP::AES> _rng;
  CryptoPP::ECDH<CryptoPP::ECP>::Domain _dh;
  CryptoPP::SecByteBlock _privKeyAes;
  CryptoPP::SecByteBlock _pubKeyAes;
  CryptoPP::SecByteBlock _key;
  CryptoPP::RSA::PrivateKey _rsaPrivKey;
  CryptoPP::RSA::PublicKey _rsaPubKey;
  CryptoPP::RSA::PublicKey _peerRsaPubKey;
  std::string _serializedRsaPubKey;
  static const CryptoPP::OID _curve;
  KeyHandler _keyHandler;
  bool _verified = false;
  bool _signatureSent = false;
  std::mutex _mutex;
  bool generateKeyPair(CryptoPP::ECDH<CryptoPP::ECP>::Domain& dh,
		       CryptoPP::SecByteBlock& priv,
		       CryptoPP::SecByteBlock& pub);
  template <typename INSTANCE>
  void setAESmodule(INSTANCE& instance) {
    std::lock_guard lock(_mutex);
    _keyHandler.recoverKey(_key);
    instance = INSTANCE(_key.data(), _key.size());
    _keyHandler.hideKey(_key);
  }
  bool checkAccess();
  void hideKey();
  void eraseRSAKeys();
  void signMessage();
  std::string sha256_hash(std::u8string_view message);
  void erasePubPrivKeys();
  bool verifySignature(std::string_view signature);
  void decodePeerRsaPublicKey(std::string_view rsaPubBserialized);

public:
  CryptoPlPl(std::string_view msgHash,
	     std::string_view pubKeyAesClient,
	     std::string_view signatureWithPubKey);
  explicit CryptoPlPl(std::u8string_view msg);
  ~CryptoPlPl() = default;
  std::string _msgHash;
  std::string _encodedPubKeyAes;
  std::string _signatureWithPubKeySign;
  void showKey();
  std::string_view encrypt(std::string& buffer, const HEADER& header, std::string_view data);
  void decrypt(std::string& buffer, std::string& data);
  bool clientKeyExchange(std::string_view encodedPeerPubKeyAes);
  std::pair<bool, std::string>
  encodeRsaPublicKey(const CryptoPP::RSA::PrivateKey& privateKey);
  bool decodeRsaPublicKey(std::string_view serializedKey,
			  CryptoPP::RSA::PublicKey& publicKey);

  std::string base64_encode(std::span<unsigned char> binary);
  std::vector<unsigned char> base64_decode(std::string_view encoded);
  void setDummyAesKey();
  template <typename L>
  bool sendSignature(L& lambda) {
    HEADER header = { HEADERTYPE::DH_INIT, _msgHash.size(), _encodedPubKeyAes.size(),
		      CRYPTO::NONE, COMPRESSORS::NONE,
		      DIAGNOSTICS::NONE, STATUS::NONE, _signatureWithPubKeySign.size() };
    bool result = lambda(header, _msgHash, _encodedPubKeyAes, _signatureWithPubKeySign);
    if (result)
      _signatureSent = true;
    eraseRSAKeys();
    return result;
  }
};
