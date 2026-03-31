/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EncryptorTemplates.h"

#pragma once

namespace cryptovariant {

bool initialize();

bool isInitialized();

CryptoVariant& getEncryptorVariant(APPTYPE app, CRYPTO crypto);

} // end of namespace cryptovariant
