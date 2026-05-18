/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoOperations.h"

#pragma once

namespace cryptotuple {

bool initialize();

bool isInitialized();

CryptoTuple getClientEncryptorTuple();
CryptoTuple getServerEncryptorTuple();

} // end of namespace cryptotuple
