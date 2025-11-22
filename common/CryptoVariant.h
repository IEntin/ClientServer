/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

namespace cryptovariant {

static std::string_view
compressEncrypt(CryptoVariant& container,
		std::string& buffer,
		const HEADER& header,
		std::string& data,
		bool doEncrypt,
		int compressionLevel = 3) {
  auto crypto = std::get<getEncryptorIndex()>(container);
  return compressEncrypt(buffer, header, doEncrypt, makeWeak(crypto), data, compressionLevel);
}

static void decryptDecompress(CryptoVariant& container,
			      std::string& buffer,
			      HEADER& header,
			      std::string& data) {
  auto crypto = std::get<getEncryptorIndex()>(container);
  return decryptDecompress(buffer, header, makeWeak(crypto), data);
}

} // end of namespace cryptovariant
