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

template <std::size_t I = 0, typename Func, typename... Types>
typename std::enable_if<I == sizeof...(Types), void>::type
runtime_get_helper([[maybe_unused]] std::size_t index, [[maybe_unused]] Func f,  [[maybe_unused]] std::variant<Types...>& t) {
  throw std::runtime_error("Index out of bounds");
}

template <std::size_t I = 0, typename Func, typename... Types>
typename std::enable_if<I < sizeof...(Types), void>::type
runtime_get_helper(std::size_t index, Func f, std::variant<Types...>& t) {
    if (index == I) {
        f(std::get<I>(t));
    } else {
        runtime_get_helper<I + 1>(index, f, t);
    }
}

template <typename Func, typename... Types>
void runtime_get(std::size_t index, Func f, std::variant<Types...>& t) {
    runtime_get_helper(index, f, t);
}

static void
getEncryptor(EncryptorVariant encryptors, CRYPTO type = Options::_encryptorTypeDefault) {
  try {
    auto index = std::to_underlying<CRYPTO>(type);
    runtime_get(index, [&]([[maybe_unused]] auto& value) {
    }, encryptors);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

} // end of namespace cryptovariant
