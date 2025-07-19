/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "CryptoPlPl.h"
#include "CryptoSodium.h"

using CryptoPtr = CryptoPlPlPtr;
using CryptoWeakPtr = CryptoWeakPlPlPtr;

inline CryptoPtr createCrypto(std::u8string_view  msg) {
  return std::make_shared<CryptoPlPl>(msg);
}

inline CryptoPtr createCrypto(std::span<const unsigned char> msgHash,
			      std::span<const unsigned char> pubB,
			      std::span<const unsigned char> signatureWithPubKey) {
  return std::make_shared<CryptoPlPl>(msgHash, pubB, signatureWithPubKey);
}
