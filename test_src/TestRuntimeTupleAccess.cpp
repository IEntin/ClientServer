// clang++ runtime_tuple_access.cpp -std=c++2b -o runtimetupleaccess
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <tuple>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "CryptoPlPl.h"
#include "CryptoSodium.h"
#include "CryptoTuple.h"
#include "Logger.h"
#include "TestEnvironment.h"

TEST(RuntimeTupleAccess, 1) {
  EncryptorTuple encryptors(std::make_shared<CryptoPlPl>(), std::make_shared<CryptoSodium>());
  unsigned long resultIndex;
  getEncryptor(encryptors, CRYPTO::CRYPTOSODIUM, resultIndex);
  ASSERT_TRUE(resultIndex == requestIndexSodium);
  getEncryptor(encryptors, CRYPTO::CRYPTOPP, resultIndex);
  ASSERT_TRUE(resultIndex == requestIndexCryptoPP);

  auto encryptorSodium = getSodiumEncryptor(encryptors);
  auto name1 = encryptorSodium->getName();
  ASSERT_TRUE(name1 == std::string("CryptoSodium"));
  auto encryptorCpp = getCryptoPLPlEncryptor(encryptors);
  auto name2 = encryptorCpp->getName();
  ASSERT_TRUE(name2 == std::string("CryptoPlPl"));
}
