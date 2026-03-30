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

CryptoVariant& getEncryptorVariant(APPTYPE app, CRYPTO crypto);

std::string_view compressEncrypt(CryptoVariant& variant,
				 std::string& buffer,
				 const HEADER& header,
				 std::string& data,
				 bool doEncrypt,
				 int compressionLevel = 3);

void decryptDecompress(APPTYPE app,
		       CRYPTO crypto,
		       std::string& buffer,
		       HEADER& header,
		       std::string& data);

} // end of namespace cryptovariant
