/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EncryptorTemplates.h"

namespace encryptortemplates {

std::string_view singleEncrypt(const CryptoTuple& tuple,
			       CRYPTO crypto,
			       std::string& buffer,
			       const HEADER& header,
			       std::string& source) {
  buffer.clear();
  switch (crypto) {
  case CRYPTO::CRYPTOSODIUM:
    {
      CryptoWeakSodiumPtr cryptoWeakSodiumPtr = std::get<CryptoWeakSodiumPtr>(tuple);
      if (CryptoSodiumPtr cryptoSodiumPtr = cryptoWeakSodiumPtr.lock()) {
	return cryptoSodiumPtr->encrypt(buffer, &header, source);
      }
    }
    break;
  case CRYPTO::CRYPTOPP:
    {
      CryptoWeakPlPlPtr cryptoWeakPlPlPtr = std::get<CryptoWeakPlPlPtr>(tuple);
      if (CryptoPlPlPtr cryptoPlPlPtr = cryptoWeakPlPlPtr.lock()) {
	return cryptoPlPlPtr->encrypt(buffer, &header, source);
      }
    }
    break;
  default:
    break;
  }
  return "";
}

void singleDecrypt(const CryptoTuple& tuple,
		   CRYPTO crypto,
		   std::string& buffer,
		   HEADER& header,
		   std::string& data) {
  switch (crypto) {
  case CRYPTO::CRYPTOSODIUM:
    {
      CryptoWeakSodiumPtr cryptoWeakSodiumPtr = std::get<CryptoWeakSodiumPtr>(tuple);
      if (CryptoSodiumPtr cryptoSodiumPtr = cryptoWeakSodiumPtr.lock())
	cryptoSodiumPtr->decrypt(buffer, data);
    }
    break;
  case CRYPTO::CRYPTOPP:
    {
      CryptoWeakPlPlPtr cryptoWeakPlPlPtr = std::get<CryptoWeakPlPlPtr>(tuple);
      if (CryptoPlPlPtr cryptoPlPlPtr  = cryptoWeakPlPlPtr.lock())
	cryptoPlPlPtr->decrypt(buffer, data);
    }
    break;
  default:
    break;
  }
  if (!deserialize(header, data.data()))
    throw std::runtime_error("deserialize failed");
  data.erase(0, HEADER_SIZE);
}

std::string doubleEncrypt(const CryptoTuple& tuple,
			  std::string& buffer,
			  const HEADER& header,
			  std::string& source) {
  std::string encrypted;
  CryptoWeakSodiumPtr cryptoWeakSodiumPtr = std::get<CryptoWeakSodiumPtr>(tuple);
  if (CryptoSodiumPtr cryptoSodiumPtr = cryptoWeakSodiumPtr.lock()) {
    buffer.clear();
    encrypted = cryptoSodiumPtr->encrypt(buffer, &header, source);
  }
  CryptoWeakPlPlPtr cryptoWeakPlPlPtr = std::get<CryptoWeakPlPlPtr>(tuple);
  if (CryptoPlPlPtr cryptoPlPlPtr = cryptoWeakPlPlPtr.lock()) {
    buffer.clear();
    encrypted = cryptoPlPlPtr->encrypt(buffer, nullptr, encrypted);
  }
  return encrypted;
}

void doubleDecrypt(const CryptoTuple& tuple,
		   std::string& buffer,
		   HEADER& header,
		   std::string& data) {
  CryptoWeakPlPlPtr cryptoWeakPlPlPtr = std::get<CryptoWeakPlPlPtr>(tuple);
  if (CryptoPlPlPtr cryptoPlPlPtr = cryptoWeakPlPlPtr.lock()) {
    buffer.clear();
    cryptoPlPlPtr->decrypt(buffer, data);
  }
  CryptoWeakSodiumPtr cryptoWeakSodiumPtr = std::get<CryptoWeakSodiumPtr>(tuple);
  if (CryptoSodiumPtr cryptoSodiumPtr = cryptoWeakSodiumPtr.lock()) {
    buffer.clear();
    cryptoSodiumPtr->decrypt(buffer, data);
  }
  if (!deserialize(header, data.data()))
    throw std::runtime_error("doubleDecrypt failure.");
}

std::string_view compressSingleEncrypt(const CryptoTuple& tuple,
				       CRYPTO crypto,
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
    return singleEncrypt(tuple, crypto, buffer, header, data);
  else
    return data.insert(0, serialize(header));
}

void singleDecryptDecompress(const CryptoTuple& tuple,
			     CRYPTO crypto,
			     std::string& buffer,
			     HEADER& header,
			     std::string& data) {
  singleDecrypt(tuple, crypto, buffer, header, data);
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
