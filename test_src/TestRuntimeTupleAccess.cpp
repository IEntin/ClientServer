/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"
#include "TestEnvironment.h"

TEST(RuntimeTupleAccess, 1) {
  cryptotuple::ENCRYPTORTUPLE encryptors = cryptotuple::createCrypto();
  auto cryptopp = cryptotuple::getEncryptor<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOPP)>(encryptors);
  ASSERT_TRUE(cryptopp->getName() == "CryptoPlPl");
  auto cryptosodium = cryptotuple::getEncryptor<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOSODIUM)>(encryptors);
  ASSERT_TRUE(cryptosodium->getName() == "CryptoSodium");
}
