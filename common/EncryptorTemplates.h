/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"
#include "CryptoPlPl.h"
#include "CryptoSodium.h"

using CryptoVariant = std::variant<CryptoSodiumPtr, CryptoPlPlPtr>;
using CryptoTuple = std::tuple<CryptoSodiumPtr, CryptoPlPlPtr>;

using ENCRYPTORCONTAINER = std::conditional_t<Options::_useEncryptorVariant, CryptoVariant, CryptoTuple>;

template <typename CONTAINER>
std::string_view compressEncrypt(CONTAINER& container,
				 std::string& buffer,
				 const HEADER& header,
				 std::string& data,
				 bool doEncrypt,
				 int compressionLevel = 3) {
  auto crypto = std::get<getEncryptorIndex()>(container);
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
      return crypto->encrypt(buffer, header, data);
    }
  }
  else {
    auto serialized = serialize(header);
    std::string headerWithData;
    headerWithData.append(serialized).append(data);
    data.swap(headerWithData);
    return data;
  }
  return "";
}
template <typename CONTAINER>
std::string_view decryptDecompress(CONTAINER& container,
				   std::string& buffer,
				   HEADER& header,
				   std::string& data) {
  auto crypto = std::get<getEncryptorIndex()>(container);
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
  return { data.data(), data.size() };
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

static CryptoSodiumPtr createServerEncryptor(CryptoSodiumPtr cryptoClient) {
  return std::make_shared<CryptoSodium>(cryptoClient->_encodedPubKeyAes,
					cryptoClient->_signatureWithPubKeySign);
}

static CryptoPlPlPtr createServerEncryptor(CryptoPlPlPtr cryptoClient) {
  return std::make_shared<CryptoPlPl>(cryptoClient->_encodedPubKeyAes,
				      cryptoClient->_signatureWithPubKeySign);
}

template <typename CONTAINER>
void fillEncryptorContainer(CONTAINER& container,
			    CRYPTO encryptorType) {
  if constexpr (std::is_same_v<CONTAINER, CryptoVariant>) {
    switch(encryptorType) {
    case CRYPTO::CRYPTOSODIUM:
      container = std::make_shared<CryptoSodium>();
      break;
    case CRYPTO::CRYPTOPP:
      container = std::make_shared<CryptoPlPl>();
      break;
    default:
      break;
    }
  }
  else if constexpr (std::is_same_v<CONTAINER, CryptoTuple>) {
    container = { std::make_shared<CryptoSodium>(), std::make_shared<CryptoPlPl>() };
  }
}

template <typename CONTAINER>
void fillEncryptorContainer(CONTAINER& container,
			    CRYPTO encryptorType,
			    std::string_view encodedPeerPubKeyAes,
 			    std::string_view signatureWithPubKey) {
 if constexpr (std::is_same_v<CONTAINER, CryptoVariant>) {
    switch(encryptorType) {
    case CRYPTO::CRYPTOPP:
      container = { std::make_shared<CryptoPlPl>(encodedPeerPubKeyAes, signatureWithPubKey) };
      break;
    case CRYPTO::CRYPTOSODIUM:
      container = { std::make_shared<CryptoSodium>(encodedPeerPubKeyAes, signatureWithPubKey) };
      break;
    default:
      break;
    }
 }
 else if constexpr (std::is_same_v<CONTAINER, CryptoTuple>) {
 extern CryptoTuple _clientEncryptorTuple;
 CryptoSodiumPtr clientEncryptor0 = std::get<0>(_clientEncryptorTuple);
 CryptoSodiumPtr serverEncryptor0 = createServerEncryptor(clientEncryptor0);
 clientEncryptor0->clientKeyExchange(serverEncryptor0->_encodedPubKeyAes);
 CryptoPlPlPtr clientEncryptor1 = std::get<1>(_clientEncryptorTuple);
 CryptoPlPlPtr serverEncryptor1 = createServerEncryptor(clientEncryptor1);
 clientEncryptor1->clientKeyExchange(serverEncryptor1->_encodedPubKeyAes);
 container = { serverEncryptor0, serverEncryptor1 };
 }
}
