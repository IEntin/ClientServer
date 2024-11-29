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
  std::string _serializedRsaPubKey;
  CryptoPP::RSA::PublicKey _peerRsaPubKey;
  std::string _buffer;
  std::string _signature;
  std::mutex _mutex;
  // temp
  const std::string_view _password;
  bool generateKeyPair(CryptoPP::ECDH<CryptoPP::ECP>::Domain& dh,
		       CryptoPP::SecByteBlock& priv,
		       CryptoPP::SecByteBlock& pub) {
    dh.GenerateKeyPair(_rng, priv, pub);
    return true;
  }

  void signPassword();

  void showKeyIv(const CryptoPP::SecByteBlock& iv);
public:
  Crypto();
  Crypto(const CryptoPP::SecByteBlock& pubB);
  ~Crypto() {}
  void encrypt(bool encrypt, std::string& data);
  void decrypt(std::string& data);
  const CryptoPP::SecByteBlock& getPubKey() const { return _pubKey; }
  std::string_view getSerializedRsaPubKey() const { return _serializedRsaPubKey; }
  std::string_view getSignature() const { return _signature; }
  bool handshake(const CryptoPP::SecByteBlock& pubAreceived);
  std::pair<bool, std::string>
  encodeRsaPublicKey(const CryptoPP::RSA::PrivateKey& privateKey);

  bool decodeRsaPublicKey(std::string_view serializedKey,
			  CryptoPP::RSA::PublicKey& publicKey);

  bool decodeRsaPeerPublicKey(std::string_view serializedKey);
  static const CryptoPP::OID _curve;
};
