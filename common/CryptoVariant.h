/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include "CryptoCommon.h"

namespace cryptovariant {

static EncryptorVariant _encryptorVar;

static void createCrypto(std::optional<CRYPTO> encryptor = std::nullopt) {
  CRYPTO encryptorType = encryptor.has_value() ? *encryptor : Options::_encryptorTypeDefault;
  switch(encryptorType) {
  case CRYPTO::CRYPTOPP:
    _encryptorVar = std::make_shared<CryptoPlPl>();
    break;
  case CRYPTO::CRYPTOSODIUM:
    _encryptorVar = std::make_shared<CryptoSodium>();
    break;
  default:
    break;
  }
}

static void createCrypto(std::string_view encodedPeerPubKeyAes,
			 std::string_view signatureWithPubKey,
			 std::optional<CRYPTO> encryptor = std::nullopt) {
  CRYPTO encryptorType = encryptor.has_value() ? *encryptor : Options::_encryptorTypeDefault;
  switch(encryptorType) {
  case CRYPTO::CRYPTOPP:
    _encryptorVar = std::make_shared<CryptoPlPl>(encodedPeerPubKeyAes, signatureWithPubKey);
    break;
  case CRYPTO::CRYPTOSODIUM:
    _encryptorVar = std::make_shared<CryptoSodium>(encodedPeerPubKeyAes, signatureWithPubKey);
    break;
  default:
    break;
  }
}

} // end of namespace cryptovariant
