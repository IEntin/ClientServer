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

class Crypto {
  Crypto() = delete;
  ~Crypto() = delete;
 public:
  static const CryptoPP::OID _curve;
  static CryptoPP::AutoSeededX917RNG<CryptoPP::AES> _rng;
  static const CryptoPP::SecByteBlock _endTag;
  static std::mutex _rngMutex;
  static CryptoPP::SecByteBlock createEndTag();

  static bool generateKeyPair(CryptoPP::ECDH<CryptoPP::ECP>::Domain& dh,
			      CryptoPP::SecByteBlock& priv,
			      CryptoPP::SecByteBlock& pub) {
    std::scoped_lock lock(_rngMutex);
    dh.GenerateKeyPair(_rng, priv, pub);
    return true;
  }

  static std::string_view encrypt(bool encrypt,
				  const CryptoPP::SecByteBlock& key,
				  std::string_view data);

  static std::string_view decrypt(const CryptoPP::SecByteBlock& key,
				  std::string_view data);

  static void showKeyIv(const CryptoPP::SecByteBlock& key,
			const CryptoPP::SecByteBlock& iv);
};
