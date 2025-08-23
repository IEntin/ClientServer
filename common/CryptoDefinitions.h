/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"
#include "CryptoPlPl.h"
#include "CryptoSodium.h"
#include "Header.h"
#include "ServerOptions.h"

namespace cryptodefinitions {

constexpr CRYPTO encryption = CRYPTO::CRYPTOPP;

static consteval unsigned long getEncryptionIndex() {
  if (encryption == CRYPTO::CRYPTOPP)
    return 0;
  else if (encryption == CRYPTO::CRYPTOSODIUM)
    return 1;
  else
    return 11;
}

static consteval unsigned long getEncryptionIndex(const CRYPTO crypto) {
  if (crypto == CRYPTO::CRYPTOPP)
    return 0;
  else if (crypto == CRYPTO::CRYPTOSODIUM)
    return 1;
  else
    return 11;
}

inline const auto sodiumInitialized = [] {
  int initialized = sodium_init();
  assert(initialized == 0 && "libsodium initialization failed");
  return 0;
 };

// expected message starts with a header
template <typename T>
bool isEncrypted(const T& input) {
  assert(input.size() >= HEADER_SIZE && "too short");
  HEADER header;
  std::string inputStr(input.cbegin(), input.cbegin() + HEADER_SIZE);
  try {
    if (deserialize(header, inputStr.data()))
      return false;
    return true;
  }
  catch (const std::runtime_error& error) {
    return true;
  }
}

inline std::variant<CryptoPlPlPtr, CryptoSodiumPtr>
createCrypto(CRYPTO encryption, std::string_view  msg) {
  std::variant<CryptoPlPlPtr, CryptoSodiumPtr> result;
  switch(encryption) {
  case CRYPTO::CRYPTOPP:
    result = std::make_shared<CryptoPlPl>(msg);
    break;
  case CRYPTO::CRYPTOSODIUM:
    result = std::make_shared<CryptoSodium>(msg);
    break;
  default:
    break;
  }
  return result;
}

inline std::variant<CryptoPlPlPtr, CryptoSodiumPtr>
createCrypto(std::string_view  msg) {
  std::variant<CryptoPlPlPtr, CryptoSodiumPtr> result;
  switch(encryption) {
  case CRYPTO::CRYPTOPP:
    result = std::make_shared<CryptoPlPl>(msg);
    break;
  case CRYPTO::CRYPTOSODIUM:
    result = std::make_shared<CryptoSodium>(msg);
    break;
  default:
    break;
  }
  return result;
}

inline std::variant<CryptoPlPlPtr, CryptoSodiumPtr>
createCrypto(CRYPTO encryption,
	     std::string_view msgHash,
	     std::string_view pubB,
	     std::string_view signatureWithPubKey) {
  std::variant<CryptoPlPlPtr, CryptoSodiumPtr> result;
  switch(encryption) {
  case CRYPTO::CRYPTOPP:
    result = std::make_shared<CryptoPlPl>(msgHash, pubB, signatureWithPubKey);
    break;
  case CRYPTO::CRYPTOSODIUM:
    result = std::make_shared<CryptoSodium>(msgHash, pubB, signatureWithPubKey);
    break;
  default:
    break;
  }
  return result;
}

inline std::variant<CryptoPlPlPtr, CryptoSodiumPtr>
createCrypto(std::string_view msgHash,
	     std::string_view pubB,
	     std::string_view signatureWithPubKey) {
  std::variant<CryptoPlPlPtr, CryptoSodiumPtr> result;
  switch(encryption) {
  case CRYPTO::CRYPTOPP:
    result = std::make_shared<CryptoPlPl>(msgHash, pubB, signatureWithPubKey);
    break;
  case CRYPTO::CRYPTOSODIUM:
    result = std::make_shared<CryptoSodium>(msgHash, pubB, signatureWithPubKey);
    break;
  default:
    break;
  }
  return result;
}

template <typename Crypto>
std::string_view compressEncrypt(std::string& buffer,
				 const HEADER& header,
				 bool doEncrypt,
				 std::weak_ptr<Crypto> weak,
				 std::string& data,
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
    if (auto crypto = weak.lock();crypto) {
      return crypto->encrypt(buffer, header, data);
    }
  }
  else {
    char headerBuffer[HEADER_SIZE] = {};
    data.insert(0, headerBuffer, HEADER_SIZE);
    serialize(header, data.data());
    return data;
  }
  return "";
}

inline std::string_view
compressEncrypt(std::variant<CryptoPlPlPtr, CryptoSodiumPtr>& cryptoVar,
		std::string& buffer,
		const HEADER& header,
		std::string& data,
		bool doEncrypt,
		int compressionLevel = 3) {
  auto crypto = std::get<getEncryptionIndex()>(cryptoVar);
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
    if (ServerOptions::_showKey)
      crypto-> showKey();
    auto weak = std::weak_ptr(crypto);
    if (auto cryptoEff = weak.lock();crypto) {
      return cryptoEff->encrypt(buffer, header, data);
    }
  }
  else {
    char headerBuffer[HEADER_SIZE] = {};
    data.insert(0, headerBuffer, HEADER_SIZE);
    serialize(header, data.data());
    return data;
  }
  return "";
}

template <typename Crypto>
void decryptDecompress(std::string& buffer,
		       HEADER& header,
		       std::weak_ptr<Crypto> weak,
		       std::string& data) {
  if (auto crypto = weak.lock();crypto) {
    crypto->decrypt(buffer, data);
    deserialize(header, data.data());
    data.erase(0, HEADER_SIZE);
    if (isCompressed(header)) {
      COMPRESSORS compressor = extractCompressor(header);
      switch (compressor) {
      case COMPRESSORS::LZ4:
	compressionLZ4::uncompress(buffer, data, extractUncompressedSize(header));
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
}

inline void decryptDecompress(std::variant<CryptoPlPlPtr, CryptoSodiumPtr>& cryptoVar,
			      std::string& buffer,
			      HEADER& header,
			      std::string& data) {
  auto crypto = std::get<getEncryptionIndex()>(cryptoVar);
  crypto->decrypt(buffer, data);
  deserialize(header, data.data());
  data.erase(0, HEADER_SIZE);
  if (isCompressed(header)) {
    COMPRESSORS compressor = extractCompressor(header);
    switch (compressor) {
    case COMPRESSORS::LZ4:
      compressionLZ4::uncompress(buffer, data, extractUncompressedSize(header));
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

} // end of namespace cryptodefinitions
