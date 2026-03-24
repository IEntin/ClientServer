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

namespace encryptortemplates {

using CryptoVariant = std::variant<CryptoSodiumPtr, CryptoPlPlPtr>;

using ENCRYPTORCONTAINER = CryptoVariant;

consteval std::size_t getEncryptorIndex(std::optional<CRYPTO> encryptor = std::nullopt) {
  CRYPTO encryptorType = encryptor.has_value() ? *encryptor : Options::_encryptorTypeDefault;
  return std::to_underlying(encryptorType);
}

template <typename CONTAINER>
auto getEncryptor(const CONTAINER& container) {
  return std::get<getEncryptorIndex()>(container);
}

template <typename CONTAINER>
std::string_view compressEncrypt(CONTAINER& container,
				 std::string& buffer,
				 const HEADER& header,
				 std::string& data,
				 bool doEncrypt,
				 int compressionLevel = 3) {
  auto crypto = getEncryptor(container);
  auto weak = makeWeak(crypto);
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
      return crypto->encrypt(buffer, &header, data);
    }
  }
  else {
    auto serialized(serialize(header));
    std::string headerWithData;
    headerWithData.append(serialized);
    headerWithData.append(data);
    data.swap(headerWithData);
    return data;
  }
  return "";
}
template <typename CONTAINER>
void decryptDecompress(CONTAINER& container,
		       std::string& buffer,
		       HEADER& header,
		       std::string& data) {
  auto crypto = getEncryptor(container);
  auto weak = makeWeak(crypto);
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

template <typename CONTAINER>
void clientKeyExchange(CONTAINER& container,
		       std::string_view encodedPeerPubKeyAes) {
  auto crypto = getEncryptor(container);
  if (!crypto->clientKeyExchange(encodedPeerPubKeyAes)) {
    throw std::runtime_error("clientKeyExchange failed");
  }
}

template <typename CONTAINER>
void sendStatusToClientImpl(CONTAINER& container,
			    std::string_view clientIdStr,
			    STATUS status,
			    HEADER& header,
			    std::string& encodedPubKeyAes) {
  auto crypto = getEncryptor(container);
  encodedPubKeyAes.assign(crypto->_encodedPubKeyAes);
  header = { HEADERTYPE::DH_HANDSHAKE, clientIdStr.size(), encodedPubKeyAes.size(),
	     COMPRESSORS::NONE, DIAGNOSTICS::NONE, status, 0 };
}

template <typename CONTAINER>
void fillEncryptorContainer(CONTAINER& container,
			    CRYPTO encryptorType) {
  switch(encryptorType) {
  case CRYPTO::CRYPTOPP:
    container = std::make_shared<CryptoPlPl>();
    break;
  case CRYPTO::CRYPTOSODIUM:
    container = std::make_shared<CryptoSodium>();
    break;
  default:
    break;
  }
}

template <typename CONTAINER>
void fillEncryptorContainer(CONTAINER& container,
			    CRYPTO encryptorType,
			    std::string_view encodedPeerPubKeyAes,
 			    std::string_view signatureWithPubKey) {
  switch(encryptorType) {
  case CRYPTO::CRYPTOPP:
    container = std::make_shared<CryptoPlPl>(encodedPeerPubKeyAes, signatureWithPubKey);
    break;
  case CRYPTO::CRYPTOSODIUM:
    container = std::make_shared<CryptoSodium>(encodedPeerPubKeyAes, signatureWithPubKey);
    break;
  default:
    break;
  }
}

} // end of namespace encryptortemplates
