// clang++ runtime_tuple_access.cpp -std=c++2b -o runtimetupleaccess
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <tuple>
#include <utility>

#include "CryptoPlPl.h"
#include "CryptoSodium.h"
#include "CryptoTuple.h"
#include "TestEnvironment.h"

TEST(RuntimeTupleAccess, 1) {
  cryptotuple::EncryptorTuple encryptors = cryptotuple::createCrypto();
  unsigned long resultIndex;
  cryptotuple::getEncryptor(encryptors, CRYPTO::CRYPTOSODIUM, resultIndex);
  ASSERT_TRUE(resultIndex == cryptotuple::requestIndexSodium);
  cryptotuple::getEncryptor(encryptors, CRYPTO::CRYPTOPP, resultIndex);
  ASSERT_TRUE(resultIndex == cryptotuple::requestIndexCryptoPP);

  auto encryptorSodium = cryptotuple::getSodiumEncryptor(encryptors);
  auto name1 = encryptorSodium->getName();
  ASSERT_TRUE(name1 == std::string("CryptoSodium"));
  auto encryptorCryptoPP = cryptotuple::getCryptoPLPlEncryptor(encryptors);
  auto name2 = encryptorCryptoPP->getName();
  ASSERT_TRUE(name2 == std::string("CryptoPlPl"));
}
