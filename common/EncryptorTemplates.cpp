/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EncryptorTemplates.h"

namespace encryptortemplates {

std::string doubleEncrypt(const CryptoTuple& tuple,
			  std::string& buffer,
			  const HEADER& header,
			  std::string& source) {
  std::string encrypted;
  CryptoWeakSodiumPtr cryptoWeakSodiumPtr = std::get<CryptoWeakSodiumPtr>(tuple);
  if (CryptoSodiumPtr encryptorSodium = cryptoWeakSodiumPtr.lock()) {
    buffer.clear();
    encrypted = encryptorSodium->encrypt(buffer, &header, source);
  }
  CryptoWeakPlPlPtr cryptoWeakPlPlPtr = std::get<CryptoWeakPlPlPtr>(tuple);
  if (CryptoPlPlPtr encryptorPlPl = cryptoWeakPlPlPtr.lock()) {
    buffer.clear();
    encrypted = encryptorPlPl->encrypt(buffer, nullptr, encrypted);
  }
  return encrypted;
}

void doubleDecrypt(const CryptoTuple& tuple,
		   std::string& buffer,
		   HEADER& header,
		   std::string& data) {
  CryptoWeakPlPlPtr cryptoWeakPlPlPtr = std::get<CryptoWeakPlPlPtr>(tuple);
  if (CryptoPlPlPtr encryptorPlPlPtr = cryptoWeakPlPlPtr.lock()) {
    buffer.clear();
    encryptorPlPlPtr->decrypt(buffer, data);
  }
  CryptoWeakSodiumPtr cryptoWeakSodiumPtr = std::get<CryptoWeakSodiumPtr>(tuple);
  if (CryptoSodiumPtr encryptorSodiumPtr = cryptoWeakSodiumPtr.lock()) {
    buffer.clear();
    encryptorSodiumPtr->decrypt(buffer, data);
  }
  if (!deserialize(header, data.data()))
    throw std::runtime_error("doubleDecrypt failure.");
}

std::string compressDoubleEncrypt(const CryptoTuple& tuple,
				  std::string& buffer,
				  const HEADER& header,
				  std::string& data,
				  bool doEncrypt,
				  int compressionLevel) {
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
  if (doEncrypt)
    return doubleEncrypt(tuple, buffer, header, data);
  else
    return data.insert(0, serialize(header));
}

void doubleDecryptDecompress(const CryptoTuple& tuple,
			     std::string& buffer,
			     HEADER& header,
			     std::string& data) {
  doubleDecrypt(tuple, buffer, header, data);
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

} // end of namespace encryptortemplates
