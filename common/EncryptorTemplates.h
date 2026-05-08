/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <variant>

#include <boost/container/static_vector.hpp>

#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"
#include "CryptoPlPl.h"
#include "CryptoSodium.h"

using CryptoVariant = std::variant<CryptoSodiumPtr, CryptoPlPlPtr>;

using CryptoTuple = std::tuple<CryptoWeakSodiumPtr, CryptoWeakPlPlPtr>;

namespace encryptortemplates {

std::string doubleEncrypt(const CryptoTuple& tuple,
			  std::string& buffer,
			  const HEADER& header,
			  std::string& source);

void doubleDecrypt(const CryptoTuple& tuple,
		   std::string& buffer,
		   HEADER& header,
		   std::string& data);
  
using ENCRYPTORCONTAINER = CryptoVariant;

template <typename CONTAINER>
std::string_view compressEncrypt(CONTAINER& container,
				 std::string& buffer,
				 const HEADER& header,
				 std::string& data,
				 bool doEncrypt,
				 int compressionLevel = 3) {
  if (isCompressed(header)) {
    COMPRESSORS compressor = extractCompressor(header);
    switch (compressor) {
    case COMPRESSORS::LZ4:
      compressionLZ4::compress(buffer, data);
      break;
    case COMPRESSORS::SNAPPY:
      compressionSnappy::compress(buffer, data);
      break;
    case COMPRESSORS::ZSTD:
      compressionZSTD::compress(buffer, data, compressionLevel);
      break;
    default:
      break;
    }
  }
  if (doEncrypt) {
    if (CryptoSodiumPtr* ptr = std::get_if<CryptoSodiumPtr>(&container)) {
      CryptoWeakSodiumPtr weak = *ptr;
      if (auto encryptor = weak.lock())
	return encryptor->encrypt(buffer, &header, data);
    }
    else if (CryptoPlPlPtr* ptr = std::get_if<CryptoPlPlPtr>(&container)) {
      CryptoWeakPlPlPtr weak = *ptr;
      if (auto encryptor = weak.lock())
	return encryptor->encrypt(buffer, &header, data);
    }
  }
  else
    return data.insert(0, serialize(header));
  return "";
}

template <typename CONTAINER>
void decryptDecompress(CONTAINER& container,
		       std::string& buffer,
		       HEADER& header,
		       std::string& data) {
  if (CryptoSodiumPtr* ptr = std::get_if<CryptoSodiumPtr>(&container)) {
    CryptoWeakSodiumPtr weak = *ptr;
    if (auto encryptor = weak.lock())
      encryptor->decrypt(buffer, data);
  }
  else if (CryptoPlPlPtr* ptr = std::get_if<CryptoPlPlPtr>(&container)) {
    CryptoWeakPlPlPtr weak = *ptr;
    if (auto encryptor = weak.lock())
      encryptor->decrypt(buffer, data);
  }
  if (!deserialize(header, data.data()))
    throw std::runtime_error("deserialize failed");
  data.erase(0, HEADER_SIZE);
  if (isCompressed(header)) {
    COMPRESSORS compressor = extractCompressor(header);
    switch (compressor) {
    case COMPRESSORS::LZ4:
      compressionLZ4::uncompress(buffer, data);
      break;
    case COMPRESSORS::SNAPPY:
      compressionSnappy::uncompress(buffer, data);
      break;
    case COMPRESSORS::ZSTD:
      compressionZSTD::uncompress(buffer, data);
      break;
    default:
      break;
    }
  }
}

std::string compressDoubleEncrypt(const CryptoTuple& tuple,
				  std::string& buffer,
				  const HEADER& header,
				  std::string& data,
				  bool doEncrypt,
				  int compressionLevel = 3);

void doubleDecryptDecompress(const CryptoTuple& tuple,
			     std::string& buffer,
			     HEADER& header,
			     std::string& data);

} // end of namespace encryptortemplates
