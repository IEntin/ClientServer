/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoPlPl.h"
#include "CryptoSodium.h"

#pragma once

using CryptoTuple = std::tuple<CryptoSodiumPtr, CryptoPlPlPtr>;

namespace cryptotuple {

bool initialize();

bool isInitialized();

CryptoTuple getClientEncryptorTuple();
CryptoTuple getServerEncryptorTuple();

} // end of namespace cryptotuple
