/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"
#include "TestEnvironment.h"

TEST(RuntimeTupleAccess, 1) {
  static CryptoSodiumPtr encryptor0 = std::make_shared<CryptoSodium>();
  static CryptoPlPlPtr encryptor1 = std::make_shared<CryptoPlPl>();
						      
  CryptoTuple encryptors = std::make_tuple(encryptor0, encryptor1 );
  CryptoWeakPlPlPtr cryptoppWeak = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOPP)>(encryptors);
  if (CryptoPlPlPtr encryptorpp = cryptoppWeak.lock()) {
    ASSERT_TRUE(encryptorpp->getName() == "CryptoPlPl");
  }
  CryptoWeakSodiumPtr cryptoWeaksodium = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOSODIUM)>(encryptors);
  if (CryptoSodiumPtr encryptorSodium = cryptoWeaksodium.lock()) {
    ASSERT_TRUE(encryptorSodium->getName() == "CryptoSodium");
  }
}
