/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/container/static_vector.hpp>

#include "Header.h"
#include "Options.h"

using EncryptorVector = boost::container::static_vector<std::shared_ptr<class CryptoBase>, 3>;

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
