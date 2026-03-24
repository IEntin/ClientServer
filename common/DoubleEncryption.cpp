/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "DoubleEncryption.h"

#include "CryptoVariant.h"

DoubleEncryption::DoubleEncryption(CRYPTO crypto0, CRYPTO crypto1) :
  _crypto0(crypto0),
  _crypto1(crypto1) {}

std::string_view DoubleEncryption::encryptDouble([[maybe_unused]] std::string& buffer,
						 [[maybe_unused]] const HEADER* const header,
						 [[maybe_unused]] std::string_view data) {
  CryptoVariant encryptorVariantClient0 = cryptovariant::getClientEncryptorVariant(_crypto0);
  CryptoVariant encryptorVariantClient1 = cryptovariant::getClientEncryptorVariant(_crypto1);
  return "";
}

void DoubleEncryption::decryptDouble([[maybe_unused]] std::string& buffer,
				     [[maybe_unused]] std::string& data) {
  CryptoVariant encryptorVariantServer0 = cryptovariant::getServerEncryptorVariant(_crypto0);
  CryptoVariant encryptorVariantServer1 = cryptovariant::getServerEncryptorVariant(_crypto1);
}
