/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "CryptoPlPl.h"
#include "CryptoSodium.h"

#ifdef _CRYPTOPP

using CryptoPtr = CryptoPlPlPtr;
using CryptoWeakPtr = CryptoWeakPlPlPtr;

inline CryptoPtr createCrypto(std::string_view  msg) {
  return std::make_shared<CryptoPlPl>(msg);
}

inline CryptoPtr createCrypto(std::string_view msgHash,
			      std::string_view pubB,
			      std::string_view signatureWithPubKey) {
  return std::make_shared<CryptoPlPl>(msgHash, pubB, signatureWithPubKey);
}

#elif _SODIUM

using CryptoPtr = CryptoSodiumPtr;
using CryptoWeakPtr = CryptoWeakSodiumPtr;


inline CryptoPtr createCrypto(std::string_view  msg) {
  return std::make_shared<CryptoSodium>(msg);
}

inline CryptoPtr createCrypto(std::string_view msgHash,
			      std::string_view pubB,
			      std::span<unsigned char> signatureWithPubKey) {
  return std::make_shared<CryptoSodium>(msgHash, pubB, signatureWithPubKey);
}

#endif
