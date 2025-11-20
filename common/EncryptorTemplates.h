/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"
#include "CryptoPlPl.h"
#include "CryptoSodium.h"

using CryptoVariant = std::variant<CryptoPlPlPtr, CryptoSodiumPtr>;

#ifdef CRYPTOVARIANT
using ENCRYPTORCONTAINER = std::variant<CryptoPlPlPtr, CryptoSodiumPtr>;;
#else
using ENCRYPTORCONTAINER = std::array<class CryptoBase, 3>;
#endif

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
    serialize(header, headerBuffer);
    std::string headerWithData;
    headerWithData.append(headerBuffer, HEADER_SIZE);
    headerWithData.append(data);
    data.swap(headerWithData);
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

template <typename CONTAINER>
void clientKeyExchange(CONTAINER& container,
		       std::string_view encodedPeerPubKeyAes) {
  auto crypto = std::get<getEncryptorIndex()>(container);
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
  auto crypto = std::get<getEncryptorIndex()>(container);
  encodedPubKeyAes.assign(crypto->_encodedPubKeyAes);
  header = { HEADERTYPE::DH_HANDSHAKE, clientIdStr.size(), encodedPubKeyAes.size(),
	     COMPRESSORS::NONE, DIAGNOSTICS::NONE, status, 0 };
}
