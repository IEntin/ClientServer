/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"
#include "TestEnvironment.h"

TEST(RuntimeTupleAccess, 1) {
  EncryptorTuple encryptors = cryptotuple::createCrypto();
  CryptoPlPlPtr valueCryptoPP;
  CryptoSodiumPtr valueCryptoSodium;
  cryptotuple::getEncryptors(encryptors, valueCryptoPP, valueCryptoSodium);
  ASSERT_TRUE(valueCryptoPP->getName() == std::string("CryptoPlPl"));
  ASSERT_TRUE(valueCryptoSodium->getName() == std::string("CryptoSodium"));

  CRYPTO crypto = CRYPTO::CRYPTOSODIUM;
  std::size_t foundIndex;
  cryptotuple::getEncryptor(encryptors, foundIndex, crypto);
  ASSERT_TRUE(foundIndex == std::to_underlying<CRYPTO>(crypto));

  auto cryptopp = cryptotuple::getTupleElement<0>(encryptors);
  ASSERT_TRUE(cryptopp->getName() == "CryptoPlPl");
  auto cryptosodium = cryptotuple::getTupleElement<1>(encryptors);
  ASSERT_TRUE(cryptosodium->getName() == "CryptoSodium");

}
