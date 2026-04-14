/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"
#include "TestEnvironment.h"

TEST(RuntimeTupleAccess, 0) {
  static CryptoSodiumPtr encryptorSodium = std::make_shared<CryptoSodium>();
  static CryptoPlPlPtr encryptorCryptoPP = std::make_shared<CryptoPlPl>();
  CryptoTuple encryptors = std::make_tuple(encryptorSodium, encryptorCryptoPP);
  CryptoWeakPlPlPtr weakCryptoPP = std::get<CryptoWeakPlPlPtr>(encryptors);
  if (CryptoPlPlPtr encryptorCryptoPP = weakCryptoPP.lock()) {
    ASSERT_TRUE(encryptorCryptoPP->getName() == "CryptoPlPl");
  }
  CryptoWeakSodiumPtr weakCryptoSodium = std::get<CryptoWeakSodiumPtr>(encryptors);
  if (CryptoSodiumPtr encryptorSodium = weakCryptoSodium.lock()) {
    ASSERT_TRUE(encryptorSodium->getName() == "CryptoSodium");
  }
}

TEST(EncryptDecrypt, 0) {
  CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();
  CryptoTuple serverTuple = cryptotuple::getServerEncryptorTuple();
  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0, 0 };
  std::string_view encrypted;
  CryptoWeakSodiumPtr weakSodiumClient = std::get<CryptoWeakSodiumPtr>(clientTuple);
  if (CryptoSodiumPtr encryptorSodiumClient = weakSodiumClient.lock())
    encrypted = encryptorSodiumClient->encrypt(TestEnvironment::_buffer,
					       &header,
					       TestEnvironment::_source);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  std::string data(encrypted);
  CryptoWeakSodiumPtr weakSodiumServer = std::get<CryptoWeakSodiumPtr>(serverTuple);
  if (CryptoSodiumPtr encryptorSodiumServer = weakSodiumServer.lock())
    encryptorSodiumServer->decrypt(TestEnvironment::_buffer, data);
  ASSERT_FALSE(CryptoBase::isEncrypted(data));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, data.data());
  ASSERT_EQ(header, recoveredHeader);
  std::string_view payload(data.cbegin() + HEADER_SIZE, data.cend());
  ASSERT_EQ(payload, TestEnvironment::_source);
}

TEST(EncryptDecrypt, 1) {
  CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();
  CryptoTuple serverTuple = cryptotuple::getServerEncryptorTuple();
  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0, 0 };
  std::string_view encrypted;
  CryptoWeakPlPlPtr weakCryptoPPClient = std::get<CryptoWeakPlPlPtr>(clientTuple);
  if (CryptoPlPlPtr encryptorCryptoPPClient = weakCryptoPPClient.lock())
      encrypted = encryptorCryptoPPClient->encrypt(TestEnvironment::_buffer,
						   &header,
						   TestEnvironment::_source);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  std::string data(encrypted);
  CryptoWeakPlPlPtr weakCryptoPPServer = std::get<CryptoWeakPlPlPtr>(serverTuple);
  if (CryptoPlPlPtr encryptorCryptoPPServer = weakCryptoPPServer.lock())
    encryptorCryptoPPServer->decrypt(TestEnvironment::_buffer, data);
  ASSERT_FALSE(CryptoBase::isEncrypted(data));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, data.data());
  ASSERT_EQ(header, recoveredHeader);
  std::string_view payload(data.cbegin() + HEADER_SIZE, data.cend());
  ASSERT_EQ(payload, TestEnvironment::_source);
}
