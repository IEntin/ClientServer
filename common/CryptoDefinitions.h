/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include "CryptoCommon.h"

namespace cryptodefinitions {

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

static void clientKeyExchange(EncryptorVariant& cryptoVar,
			      std::string_view encodedPeerPubKeyAes) {
  auto crypto = std::get<cryptocommon::getEncryptorIndex()>(cryptoVar);
  if (!crypto->clientKeyExchange(encodedPeerPubKeyAes)) {
    throw std::runtime_error("clientKeyExchange failed");
  }
}

static void sendStatusToClient(EncryptorVariant& cryptoVar,
			       std::string_view clientIdStr,
			       STATUS status,
			       HEADER& header,
			       std::string& encodedPubKeyAes) {
  auto crypto = std::get<cryptocommon::getEncryptorIndex()>(cryptoVar);
  encodedPubKeyAes.assign(crypto->_encodedPubKeyAes);
  header = { HEADERTYPE::DH_HANDSHAKE, clientIdStr.size(), encodedPubKeyAes.size(),
	     COMPRESSORS::NONE, DIAGNOSTICS::NONE, status, 0 };
}

} // end of namespace cryptodefinitions
