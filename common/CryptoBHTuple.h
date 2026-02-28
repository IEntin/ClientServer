/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <boost/hana.hpp>

#include "CryptoPlPl.h"
#include "CryptoSodium.h"

#pragma once

using CryptoBHTuple = boost::hana::tuple<CryptoSodiumPtr, CryptoPlPlPtr>;

namespace cryptobhtuple {

bool initialize();

bool isInitialized();

CryptoBHTuple getClientEncryptors();
CryptoBHTuple getServerEncryptors();

inline auto getTupleElement(const CryptoBHTuple& tuple, CRYPTO cryptoType) {
auto lambda = [](auto const& v) {
  return v;
};

unsigned index = std::to_underlying<CRYPTO>(cryptoType);
unsigned i = 0;
boost::hana::for_each(tuple, [&](auto& el) {
  if (i == index)
    lambda(el);
  i++;
 });
}

} // end of namespace cryptobhtuple
