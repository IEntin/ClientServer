/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoPlPl.h"
#include "CryptoSodium.h"

#pragma once

using CryptoVariant = std::variant<CryptoSodiumPtr, CryptoPlPlPtr>;

namespace cryptovariant {

bool initialize();

bool isInitialized();

const CryptoVariant& getClientEncryptorVariant(CRYPTO crypto);
const CryptoVariant& getServerEncryptorVariant(CRYPTO crypto);

} // end of namespace cryptovariant
