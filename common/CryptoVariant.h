/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoPlPl.h"
#include "CryptoSodium.h"

#pragma once

using CryptoVariant = std::variant<CryptoSodiumPtr, CryptoPlPlPtr>;

namespace cryptovariant {

  constexpr std::size_t _index0 = 0;
  constexpr std::size_t _index1 = 1;

bool initialize();

bool isInitialized();

const CryptoVariant& getClientEncryptorVariant(CRYPTO crypto);
const CryptoVariant& getServerEncryptorVariant(CRYPTO crypto);

CryptoSodiumPtr getCryptoSodiumValue(CryptoVariant& var);
CryptoPlPlPtr getCryptoPPValue(CryptoVariant& var);

template <typename CRYPTO>
const auto& getClientEncryptor(CRYPTO crypto) {
  CryptoVariant variant = getClientEncryptorVariant(crypto);
  if constexpr (crypto == CRYPTO::CRYPTOSODIUM)
    return getCryptoSodiumValue(variant);
  else if constexpr (crypto == CRYPTO::CRYPTOPP)
    return getCryptoPPValue(variant);
}

template <typename CRYPTO>
const auto& getServerEncryptor(CRYPTO crypto) {
  CryptoVariant variant = getServerEncryptorVariant(crypto);
  if constexpr (crypto == CRYPTO::CRYPTOSODIUM)
    return getCryptoSodiumValue(variant);
  else if constexpr (crypto == CRYPTO::CRYPTOPP)
    return getCryptoPPValue(variant);
}

template <typename CRYPTO>
const auto& getEncryptor(CRYPTO crypto, CryptoVariant& variant) {
  if constexpr (crypto == CRYPTO::CRYPTOSODIUM)
    return getCryptoSodiumValue(variant);
  else if constexpr (crypto == CRYPTO::CRYPTOPP)
    return getCryptoPPValue(variant);
}

} // end of namespace cryptovariant
