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

constexpr std::u8string_view endTag = u8"r2345ufg5432105t";
constexpr std::size_t rsaKeySize = 2048;
constexpr std::size_t SIGNATURE_SIZE = 256;

using CryptoPtr = std::shared_ptr<class Crypto>;

using CryptoWeakPtr = std::weak_ptr<class Crypto>;

class Crypto {
  CryptoPP::AutoSeededX917RNG<CryptoPP::AES> _rng;
  CryptoPP::ECDH<CryptoPP::ECP>::Domain _dh;
  CryptoPP::SecByteBlock _privKey;
  CryptoPP::SecByteBlock _pubKey;
  CryptoPP::SecByteBlock _key;
  CryptoPP::RSA::PrivateKey _rsaPrivKey;
  CryptoPP::RSA::PublicKey _rsaPubKey;
  CryptoPP::RSA::PublicKey _peerRsaPubKey;
  std::string _serializedRsaPubKey;
  std::string _signatureWithPubKey;
  static const CryptoPP::OID _curve;
  // temp
  const std::string_view _message;
  bool generateKeyPair(CryptoPP::ECDH<CryptoPP::ECP>::Domain& dh,
		       CryptoPP::SecByteBlock& priv,
		       CryptoPP::SecByteBlock& pub) {
    dh.GenerateKeyPair(_rng, priv, pub);
    return true;
  }
  void showKeyIv(const CryptoPP::SecByteBlock& iv);
public:
  Crypto();
  Crypto(const CryptoPP::SecByteBlock& pubB);
  ~Crypto();
  void encrypt(std::string& buffer, bool encrypt, std::string& data);
  void decrypt(std::string& buffer, std::string& data);
  const CryptoPP::SecByteBlock& getPubKey() const { return _pubKey; }
  std::string_view getSignatureWithPubKey() const { return _signatureWithPubKey; }
  bool handshake(const CryptoPP::SecByteBlock& pubAreceived);
  std::pair<bool, std::string>
  encodeRsaPublicKey(const CryptoPP::RSA::PrivateKey& privateKey);
  void decodePeerRsaPublicKey(std::string_view rsaPubBserialized);
  void signMessage();
  bool verifySignature(const std::string& signature);
  bool decodeRsaPublicKey(std::string_view serializedKey,
			  CryptoPP::RSA::PublicKey& publicKey);
};
