/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Options.h"

// used in tests to create server locally
template <typename E>
E createServerEncryptor(E clientEncryptor) {
  // do not send siganture, it is passed through the server constructor:
  clientEncryptor->markSignatureSent();
  return std::make_shared<typename std::remove_pointer<decltype(clientEncryptor.get())>::type>(
    clientEncryptor->_encodedPubKeyAes, clientEncryptor->_signatureWithPubKeySign);
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
  void markSignatureSent() {
    _signatureSent = true;
  }
  // expected: message starts with a header
  // header is encrypted as the rest of data
  // but never compressed because decompression
  // needs header
  static bool isEncrypted(std::string_view input);

  static void displayCryptoLibName();
};
