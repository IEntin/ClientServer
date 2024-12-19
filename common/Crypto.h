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

#include <botan/auto_rng.h>
#include <botan/rsa.h>
#include <botan/types.h>

constexpr std::u8string_view endTag = u8"r2345ufg5432105t";
constexpr std::size_t RSA_KEY_SIZE = 2048;
constexpr std::size_t SIGNATURE_SIZE = 256;
const std::string TEST_MESSAGE("This message will be hashed!");

using CryptoPtr = std::shared_ptr<class Crypto>;

using CryptoWeakPtr = std::weak_ptr<class Crypto>;

class Crypto {
  CryptoPP::AutoSeededX917RNG<CryptoPP::AES> _rng;
  CryptoPP::ECDH<CryptoPP::ECP>::Domain _dh;
  CryptoPP::SecByteBlock _privKey;
  CryptoPP::SecByteBlock _pubKey;
  CryptoPP::SecByteBlock _key;
  Botan::AutoSeeded_RNG _botanRng;
  Botan::RSA_PrivateKey _privRSAkey;
  Botan::RSA_PublicKey _pubRSAKey;
  std::unique_ptr<Botan::RSA_PublicKey> _rsaPeerPublicKey;
  std::string _serializedRsaPubKey;
  std::string _signatureWithPubKey;
  static const CryptoPP::OID _curve;
  // temp
  const std::string _message;
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
  void signMessage();
  bool verifySignature(std::span<uint8_t> signature);
  bool verifySignature(std::span<uint8_t> signature,
		       const Botan::RSA_PublicKey& rsaPeerPublicKey);
  std::string hashMessage(const std::string& msg);
  bool encodeRsaPublicKey();
  std::pair<bool, std::vector<uint8_t>>
  encodeRsaPublicKey(Botan::RSA_PublicKey& publicKey);
  bool decodeRsaPublicKey(std::span<const uint8_t> keyBits);
  std::unique_ptr<Botan::RSA_PublicKey>
  deserializeRsaPublicKey(std::span<const uint8_t> keyBits);
};
