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

// override global declaration mostly for tests
using ENCRYPTORTUPLE = std::tuple<CryptoPlPlPtr, CryptoSodiumPtr>;

template <size_t Index>
auto getEncryptor(ENCRYPTORTUPLE tuple) {
  return std::get<Index>(tuple);
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
} // end of namespace cryptotuple
