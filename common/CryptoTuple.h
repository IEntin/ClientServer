/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include "CryptoCommon.h"
#include "Logger.h"

namespace cryptotuple {

template <size_t Index>
auto getTupleElement(EncryptorTuple tuple) {
  return std::get<Index>(tuple);
}

static auto getEncryptors = [](EncryptorTuple tuple, auto& valueCryptoPP, auto& valueCryptoSodium) {
  valueCryptoPP = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOPP)>(tuple);
  valueCryptoSodium = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOSODIUM)>(tuple);
};

static auto createCrypto() {
  EncryptorTuple encryptorTuple = { std::make_shared<CryptoPlPl>(), std::make_shared<CryptoSodium>() };
  return encryptorTuple;
}

static auto createCrypto(std::string_view encodedPeerPubKeyAes,
			 std::string_view signatureWithPubKey) {
  EncryptorTuple encryptorTuple = { std::make_shared<CryptoPlPl>(encodedPeerPubKeyAes, signatureWithPubKey),
				    std::make_shared<CryptoSodium>(encodedPeerPubKeyAes, signatureWithPubKey) };
  return encryptorTuple;
}

template <std::size_t I = 0, typename Func, typename... Types>
typename std::enable_if<I == sizeof...(Types), void>::type
runtime_get_helper([[maybe_unused]] std::size_t index, [[maybe_unused]] Func f,  [[maybe_unused]] std::tuple<Types...>& t) {
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

static void
getEncryptor(EncryptorTuple encryptors, std::size_t& foundIndex, CRYPTO type = Options::_encryptorTypeDefault) {
  try {
    auto index = std::to_underlying<CRYPTO>(type);
    
    runtime_get(index, [&](auto& value) {
      foundIndex = index;
      LogAlways << "\n\n" << "Value at index " << index << ": " << value->getName() << "\n\n";
    }, encryptors);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

} // end of namespace cryptotuple
