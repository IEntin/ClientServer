/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"

TEST(RuntimeTupleAccess, 1) {
  auto encryptors = std::make_tuple(std::make_shared<CryptoSodium>(), std::make_shared<CryptoPlPl>());
  auto cryptopp = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOPP)>(encryptors);
  ASSERT_TRUE(cryptopp->getName() == "CryptoPlPl");
  auto cryptosodium = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOSODIUM)>(encryptors);
  ASSERT_TRUE(cryptosodium->getName() == "CryptoSodium");
}
