/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include "CryptoCommon.h"

namespace cryptovariant {

static auto createCrypto(std::optional<CRYPTO> encryptor = std::nullopt) {
  CRYPTO encryptorType = encryptor.has_value() ? *encryptor : Options::_encryptorTypeDefault;
  EncryptorVariant encryptorVar;
  switch(encryptorType) {
  case CRYPTO::CRYPTOPP:
    encryptorVar = std::make_shared<CryptoPlPl>();
    break;
  case CRYPTO::CRYPTOSODIUM:
    encryptorVar = std::make_shared<CryptoSodium>();
    break;
  default:
    break;
  }
  return encryptorVar;
}

static auto createCrypto(std::string_view encodedPeerPubKeyAes,
			 std::string_view signatureWithPubKey,
			 std::optional<CRYPTO> encryptor = std::nullopt) {
  CRYPTO encryptorType = encryptor.has_value() ? *encryptor : Options::_encryptorTypeDefault;
  EncryptorVariant encryptorVar;
  switch(encryptorType) {
  case CRYPTO::CRYPTOPP:
    encryptorVar = std::make_shared<CryptoPlPl>(encodedPeerPubKeyAes, signatureWithPubKey);
    break;
  case CRYPTO::CRYPTOSODIUM:
    encryptorVar = std::make_shared<CryptoSodium>(encodedPeerPubKeyAes, signatureWithPubKey);
    break;
  default:
    break;
  }
  return encryptorVar;
}

} // end of namespace cryptovariant
