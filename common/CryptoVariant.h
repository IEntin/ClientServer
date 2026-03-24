/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoPlPl.h"
#include "CryptoSodium.h"

#pragma once

using CryptoVariant = std::variant<CryptoSodiumPtr, CryptoPlPlPtr>;

namespace cryptovariant {

  constexpr std::size_t _index0 = 0;
  constexpr std::size_t _index1 = 1;

bool initialize();

bool isInitialized();

const CryptoVariant& getClientEncryptorVariant(CRYPTO crypto);
const CryptoVariant& getServerEncryptorVariant(CRYPTO crypto);

std::string_view compressEncrypt(CryptoVariant& variant,
				 std::string& buffer,
				 const HEADER& header,
				 std::string& data,
				 bool doEncrypt,
				 int compressionLevel = 3);

void decryptDecompress(CryptoVariant& variant,
		       std::string& buffer,
		       HEADER& header,
		       std::string& data);

} // end of namespace cryptovariant
