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

} // end of namespace cryptobhtuple
