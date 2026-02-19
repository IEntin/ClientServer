/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Options.h"

template<typename C>
auto makeWeak(std::shared_ptr<C> crypto) {
  return std::weak_ptr<C>(crypto);
}

class CryptoBase {
 protected:
  CryptoBase() {}
  virtual std::string_view getName() const = 0;
  bool _verifiedSignature = false;
  bool _keysExchanged = false;
  bool _signatureSent = false;
  std::mutex _mutex;
public:
  virtual ~CryptoBase() = default;
// expected: message starts with a header
// header is encrypted as the rest of data
// but never compressed because decompression
// needs header
  static bool isEncrypted(std::string_view input);

  static bool displayCryptoLibName();
};
