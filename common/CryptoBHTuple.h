/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <boost/hana.hpp>

#include "CryptoPlPl.h"
#include "CryptoSodium.h"

#pragma once

using CRYPTOBHTUPLE = boost::hana::tuple<CryptoSodiumPtr, CryptoPlPlPtr>;

namespace cryptobhtuple {

bool initialize();

CRYPTOBHTUPLE getClientEncryptors();
CRYPTOBHTUPLE getServerEncryptors();

} // end of namespace cryptotuple
