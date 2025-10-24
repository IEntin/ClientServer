/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <any>
#include <string>

#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"
#include "CryptoPlPl.h"
#include "CryptoSodium.h"
#include "Header.h"

namespace cryptodefinitions {

static std::variant<CryptoPlPlPtr, CryptoSodiumPtr> _encryptorVar;
static std::tuple<CryptoPlPlPtr, CryptoSodiumPtr> _encryptors;

constexpr CRYPTO _encryptorDefault = CRYPTO::CRYPTOSODIUM;

static consteval unsigned long getEncryptorIndex(std::optional<CRYPTO> encryptor = std::nullopt) {
  CRYPTO encryptorType = encryptor.has_value() ? *encryptor : _encryptorDefault;
  return std::to_underlying(encryptorType);
}

std::any getEncryptor(std::tuple<CryptoPlPlPtr, CryptoSodiumPtr> tuple,
		      std::optional<CRYPTO> encryptor = std::nullopt);

std::any getEncryptor(std::variant<CryptoPlPlPtr, CryptoSodiumPtr> var,
		      std::optional<CRYPTO> encryptor = std::nullopt);

static auto getCryptoPP = [] (std::any cryptoAny) {
  try {
    return std::any_cast<CryptoPlPlPtr>(cryptoAny);
  }
  catch (const std::bad_any_cast& e) {
    return CryptoPlPlPtr();
  }
 };

static auto getCryptoSodium = [] (std::any cryptoAny) {
  try {
    return std::any_cast<CryptoSodiumPtr>(cryptoAny);
  }
  catch (const std::bad_any_cast& e) {
    return CryptoSodiumPtr();
  }
 };

bool initialize();

// expected: message starts with a header
// header is encrypted as the rest of data
// but never compressed because decompression
// needs header
bool isEncrypted(std::string_view input);

static void createCrypto(std::optional<CRYPTO> encryptor = std::nullopt) {
  CRYPTO encryptorType = encryptor.has_value() ? *encryptor : _encryptorDefault;
  switch(encryptorType) {
  case CRYPTO::CRYPTOPP:
    _encryptorVar = std::make_shared<CryptoPlPl>();
    break;
  case CRYPTO::CRYPTOSODIUM:
    _encryptorVar = std::make_shared<CryptoSodium>();
    break;
  default:
    break;
  }
}

static void createCrypto(std::string_view encodedPeerPubKeyAes,
			 std::string_view signatureWithPubKey,
			 std::optional<CRYPTO> encryptor = std::nullopt) {
  CRYPTO encryptorType = encryptor.has_value() ? *encryptor : _encryptorDefault;
  switch(encryptorType) {
  case CRYPTO::CRYPTOPP:
    _encryptorVar = std::make_shared<CryptoPlPl>(encodedPeerPubKeyAes, signatureWithPubKey);
    break;
  case CRYPTO::CRYPTOSODIUM:
    _encryptorVar = std::make_shared<CryptoSodium>(encodedPeerPubKeyAes, signatureWithPubKey);
    break;
  default:
    break;
  }
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
    char headerBuffer[HEADER_SIZE];
    data.insert(0, headerBuffer, HEADER_SIZE);
    serialize(header, data.data());
    return data;
  }
  return "";
}

static std::string_view
compressEncrypt(std::variant<CryptoPlPlPtr, CryptoSodiumPtr>& cryptoVar,
		std::string& buffer,
		const HEADER& header,
		std::string& data,
		bool doEncrypt,
		int compressionLevel = 3) {
  auto crypto = std::get<getEncryptorIndex()>(cryptoVar);
  
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
    return crypto->encrypt(buffer, header, data);
  else {
    std::string headerBuffer(HEADER_SIZE, '\0');
    data.insert(0, headerBuffer);
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
}

static void decryptDecompress(std::variant<CryptoPlPlPtr, CryptoSodiumPtr>& cryptoVar,
			      std::string& buffer,
			      HEADER& header,
			      std::string& data) {
  auto crypto = std::get<getEncryptorIndex()>(cryptoVar);
  crypto->decrypt(buffer, data);
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

template <typename Crypto>
void clientKeyExchange(Crypto crypto, std::string_view encodedPubKeyAes) {
  if (!crypto->clientKeyExchange(encodedPubKeyAes))
    throw std::runtime_error("clientKeyExchange failed");
}

static void clientKeyExchange(std::variant<CryptoPlPlPtr, CryptoSodiumPtr>& cryptoVar,
			      std::string_view encodedPeerPubKeyAes) {
  auto crypto = std::get<getEncryptorIndex()>(cryptoVar);
  if (!crypto->clientKeyExchange(encodedPeerPubKeyAes)) {
    throw std::runtime_error("clientKeyExchange failed");
  }
}

static void sendStatusToClient(std::variant<CryptoPlPlPtr, CryptoSodiumPtr>& cryptoVar,
			       std::string_view clientIdStr,
			       STATUS status,
			       HEADER& header,
			       std::string& encodedPubKeyAes) {
  auto crypto = std::get<getEncryptorIndex()>(cryptoVar);
  encodedPubKeyAes.assign(crypto->_encodedPubKeyAes);
  header = { HEADERTYPE::DH_HANDSHAKE, clientIdStr.size(), encodedPubKeyAes.size(),
	     COMPRESSORS::NONE, DIAGNOSTICS::NONE, status, 0 };
}

} // end of namespace cryptodefinitions
