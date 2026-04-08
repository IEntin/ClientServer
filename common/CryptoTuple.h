/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoPlPl.h"
#include "CryptoSodium.h"

#pragma once

using CryptoTuple = std::tuple<CryptoWeakSodiumPtr, CryptoWeakPlPlPtr>;

namespace cryptotuple {

bool initialize();

bool isInitialized();

CryptoTuple getClientEncryptorTuple();
CryptoTuple getServerEncryptorTuple();

std::string doubleEncrypt(const CryptoTuple& tuple,
			  std::string& buffer,
			  const HEADER& header,
			  std::string& source);

std::string doubleDecrypt(const CryptoTuple& tuple,
			  std::string& buffer,
			  HEADER& header,
			  std::string& data);

} // end of namespace cryptotuple
