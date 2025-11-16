/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/container/static_vector.hpp>

#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"
#include "Header.h"
#include "Options.h"

using ENCRYPTORVECTOR = boost::container::static_vector<std::shared_ptr<class CryptoBase>, 3>;

consteval std::size_t getEncryptorIndex(std::optional<CRYPTO> encryptor = std::nullopt) {
  CRYPTO encryptorType = encryptor.has_value() ? *encryptor : Options::_encryptorTypeDefault;
  return std::to_underlying(encryptorType);
}

class CryptoBase {
 protected:
  CryptoBase() {}
  virtual ~CryptoBase() = default;
  virtual const std::string&  getName() const = 0;
  bool _verified = false;
  bool _signatureSent = false;
  std::mutex _mutex;
public:
// expected: message starts with a header
// header is encrypted as the rest of data
// but never compressed because decompression
// needs header
  static bool isEncrypted(std::string_view input);
  static bool displayCryptoLibName();
};

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
