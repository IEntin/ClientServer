/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <any>

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

template <typename A>
A& returnResult(A& alternative) {
  return alternative;
};

inline auto getTupleElement(const CryptoBHTuple& tuple) {
 CryptoSodiumPtr alternative0 = boost::hana::at_c<0>(tuple);
 CryptoPlPlPtr alternative1 = boost::hana::at_c<1>(tuple);
 CRYPTO crypto = Options::_encryptorType;
 switch(crypto) {
 case CRYPTO::CRYPTOSODIUM:
   returnResult(alternative0);
   break;
 case CRYPTO::CRYPTOPP:
   returnResult(alternative1);
   break;
 default:
   break;
 }
}

} // end of namespace cryptobhtuple
