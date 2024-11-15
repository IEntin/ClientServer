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

constexpr std::u8string_view endTagString = u8"r2345ufg5432105t";

using CryptoPtr = std::shared_ptr<class CryptoNS>;

using CryptoWeakPtr = std::weak_ptr<class CryptoNS>;

class CryptoNS {
  CryptoPP::AutoSeededX917RNG<CryptoPP::AES> _rng;
  CryptoPP::ECDH<CryptoPP::ECP>::Domain _dh;
  CryptoPP::SecByteBlock _privKey;
  CryptoPP::SecByteBlock _pubKey;
  const bool _generatedKeyPair;
  CryptoPP::SecByteBlock _key;
  bool generateKeyPair(CryptoPP::ECDH<CryptoPP::ECP>::Domain& dh,
		       CryptoPP::SecByteBlock& priv,
		       CryptoPP::SecByteBlock& pub) {
    dh.GenerateKeyPair(_rng, priv, pub);
    return true;
  }
  void showKeyIv(const CryptoPP::SecByteBlock& iv);
public:
  CryptoNS(const CryptoPP::SecByteBlock& pubB);
  CryptoNS();
  ~CryptoNS() {}
  std::string_view encrypt(bool encrypt, std::string_view data);
  std::string_view decrypt(std::string_view data);
  const CryptoPP::SecByteBlock& getPubKey() const { return _pubKey; }
  bool handshake(const CryptoPP::SecByteBlock& pubAreceived);
  static const CryptoPP::OID _curve;
private:
};
