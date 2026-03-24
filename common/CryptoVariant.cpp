
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoVariant.h"
#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"

namespace cryptovariant {

bool _initialized = false;

CryptoVariant _clientEncryptorVariant0;
CryptoVariant _serverEncryptorVariant0;
CryptoVariant _clientEncryptorVariant1;
CryptoVariant _serverEncryptorVariant1;

bool isInitialized(){
  return _initialized;
}

bool initialize() {
  CryptoSodiumPtr clientEncryptor0 = std::make_shared<CryptoSodium>();
  CryptoSodiumPtr serverEncryptor0 = createServerEncryptor(clientEncryptor0);
  clientEncryptor0->clientKeyExchange(serverEncryptor0->_encodedPubKeyAes);
  CryptoPlPlPtr clientEncryptor1 = std::make_shared<CryptoPlPl>();
  CryptoPlPlPtr serverEncryptor1 = createServerEncryptor(clientEncryptor1);
  clientEncryptor1->clientKeyExchange(serverEncryptor1->_encodedPubKeyAes);
  _clientEncryptorVariant0 = std::move(clientEncryptor0);
  _serverEncryptorVariant0 = std::move(serverEncryptor0);
  _clientEncryptorVariant1 = std::move(clientEncryptor1);
  _serverEncryptorVariant1 = std::move(serverEncryptor1);
  _initialized = true;
  return true;
}

const CryptoVariant& getClientEncryptorVariant(CRYPTO crypto) {
  if (!isInitialized())
    initialize();
  switch(crypto) {
  case CRYPTO::CRYPTOSODIUM:
    return _clientEncryptorVariant0;
  case CRYPTO::CRYPTOPP:
    return _clientEncryptorVariant1;
  default:
    throw std::runtime_error("Bad variant index");
  }
}

const CryptoVariant& getServerEncryptorVariant(CRYPTO crypto) {
  if (!isInitialized())
    initialize();
  switch(crypto) {
  case CRYPTO::CRYPTOSODIUM:
    return _serverEncryptorVariant0;
  case CRYPTO::CRYPTOPP:
    return _serverEncryptorVariant1;
  default:
    throw std::runtime_error("Bad variant index");
  }
}

std::string_view compressEncrypt(CryptoVariant& variant,
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
  if (doEncrypt) {
    if (CryptoSodiumPtr* ptr = std::get_if<CryptoSodiumPtr>(&variant))
      return (*ptr)->encrypt(buffer, &header, data);
    else if (CryptoPlPlPtr* ptr = std::get_if<CryptoPlPlPtr>(&variant))
      return (*ptr)->encrypt(buffer, &header, data);
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

void decryptDecompress(CryptoVariant& variant,
		       std::string& buffer,
		       HEADER& header,
		       std::string& data) {
  if (CryptoSodiumPtr* ptr = std::get_if<CryptoSodiumPtr>(&variant))
    (*ptr)->decrypt(buffer, data);
  else if (CryptoPlPlPtr* ptr = std::get_if<CryptoPlPlPtr>(&variant))
    (*ptr)->decrypt(buffer, data);
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

} // end of namespace cryptovariant
