/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <tuple>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "CryptoPlPl.h"
#include "Logger.h"
#include "CryptoSodium.h"

using EncryptorTuple = std::tuple<CryptoPlPlPtr, CryptoSodiumPtr>;

unsigned long constexpr requestIndexSodium = std::to_underlying<CRYPTO>(CRYPTO::CRYPTOSODIUM);
unsigned long constexpr requestIndexCryptoPP = std::to_underlying<CRYPTO>(CRYPTO::CRYPTOPP);

CryptoSodiumPtr getSodiumEncryptor(EncryptorTuple encryptors) {
  return std::get<requestIndexSodium>(encryptors);
}

CryptoPlPlPtr getCryptoPLPlEncryptor(EncryptorTuple encryptors) {
  return std::get<requestIndexCryptoPP>(encryptors);
}

// more generic code below used only in the test
template <std::size_t I = 0, typename Func, typename... Types>
typename std::enable_if<I == sizeof...(Types), void>::type
runtime_get_helper(std::size_t index [[maybe_unused]], Func f [[maybe_unused]], std::tuple<Types...>& t [[maybe_unused]]) {
    throw std::runtime_error("Index out of bounds");
}

template <std::size_t I = 0, typename Func, typename... Types>
typename std::enable_if<I < sizeof...(Types), void>::type
runtime_get_helper(std::size_t index, Func f, std::tuple<Types...>& t) {
    if (index == I) {
        f(std::get<I>(t));
    } else {
        runtime_get_helper<I + 1>(index, f, t);
    }
}

template <typename Func, typename... Types>
void runtime_get(std::size_t index, Func f, std::tuple<Types...>& t) {
    runtime_get_helper(index, f, t);
}

void getEncryptor(EncryptorTuple encryptors, CRYPTO type, unsigned long& foundIndex) {
  try {
    auto index = std::to_underlying<CRYPTO>(type);
    
    runtime_get(index, [&](auto& value) {
      foundIndex = index;
      LogAlways << "\n\n" << "Value at index " << index << ": " << value->getName() << "\n\n";
    }, encryptors);
  }
  catch (const std::exception& e) {
  }
}
