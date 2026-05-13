/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <variant>

#include "EncryptorTemplates.h"

using CryptoVariant = std::variant<CryptoSodiumPtr, CryptoPlPlPtr>;

#pragma once

namespace cryptovariant {

bool initialize();

bool isInitialized();

CryptoVariant& getEncryptorVariant(APPTYPE app, CRYPTO crypto);

} // end of namespace cryptovariant
