/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=DoubleEncryptDecrypt*; done

TEST(RuntimeTupleAccess, 0) {
  static CryptoSodiumPtr encryptorSodium = std::make_shared<CryptoSodium>();
  static CryptoPlPlPtr encryptorPP = std::make_shared<CryptoPlPl>();
  auto encryptors = std::make_tuple(encryptorSodium, encryptorPP);
  CryptoWeakPlPlPtr cryptoppWeak = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOPP)>(encryptors);
  if (CryptoPlPlPtr cryptopp = cryptoppWeak.lock()) {
    ASSERT_TRUE(cryptopp->getName() == "CryptoPlPl");
  }
  CryptoWeakSodiumPtr cryptosodWeak = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOSODIUM)>(encryptors);
  if (CryptoSodiumPtr cryptosod = cryptosodWeak.lock()) {
    ASSERT_TRUE(cryptosod->getName() == "CryptoSodium");
  }
}

TEST(EncryptDecrypt, 0) {
  CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();
  CryptoWeakSodiumPtr cryptoWeakC0 = std::get<CryptoWeakSodiumPtr>(clientTuple);
  CryptoTuple serverTuple = cryptotuple::getServerEncryptorTuple();
  CryptoWeakSodiumPtr cryptoWeakS0 = std::get<CryptoWeakSodiumPtr>(serverTuple);
  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  std::string_view encrypted;
  if (CryptoSodiumPtr encryptor = cryptoWeakC0.lock())
    encrypted = encryptor->encrypt(TestEnvironment::_buffer,
				   &header,
				   TestEnvironment::_source);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  std::string data(encrypted);
  if (CryptoSodiumPtr encryptorsod = cryptoWeakS0.lock())
    encryptorsod->decrypt(TestEnvironment::_buffer, data);
  ASSERT_FALSE(CryptoBase::isEncrypted(data));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, data.data());
  ASSERT_EQ(header, recoveredHeader);
  std::string_view payload(data.cbegin() + HEADER_SIZE, data.cend());
  ASSERT_EQ(payload, TestEnvironment::_source);
}

TEST(EncryptDecrypt, 1) {
  CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();
  CryptoWeakPlPlPtr cryptoWeakC1 = std::get<CryptoWeakPlPlPtr>(clientTuple);
  CryptoTuple serverTuple = cryptotuple::getServerEncryptorTuple();
  CryptoWeakPlPlPtr cryptoWeakS1 = std::get<CryptoWeakPlPlPtr>(serverTuple);
  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  std::string_view encrypted;
  if (CryptoPlPlPtr encryptorpp = cryptoWeakC1.lock())
      encrypted = encryptorpp->encrypt(TestEnvironment::_buffer,
				       &header,
				       TestEnvironment::_source);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  std::string data(encrypted);
  if (CryptoPlPlPtr encryptorpp = cryptoWeakS1.lock())
  encryptorpp->decrypt(TestEnvironment::_buffer, data);
  ASSERT_FALSE(CryptoBase::isEncrypted(data));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, data.data());
  ASSERT_EQ(header, recoveredHeader);
  std::string_view payload(data.cbegin() + HEADER_SIZE, data.cend());
  ASSERT_EQ(payload, TestEnvironment::_source);
}

TEST(DoubleEncryptDecrypt, 0) {
  CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();
  CryptoWeakSodiumPtr cryptoWeakC0 = std::get<CryptoWeakSodiumPtr>(clientTuple);
  CryptoWeakPlPlPtr cryptoWeakC1 = std::get<CryptoWeakPlPlPtr>(clientTuple);
  CryptoTuple serverTuple = cryptotuple::getServerEncryptorTuple();
  CryptoWeakSodiumPtr cryptoWeakS0 = std::get<CryptoWeakSodiumPtr>(serverTuple);
  CryptoWeakPlPlPtr cryptoWeakS1 = std::get<CryptoWeakPlPlPtr>(serverTuple);

  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  std::string_view encrypted;
  if (CryptoSodiumPtr encryptorsod = cryptoWeakS0.lock())
    encrypted = encryptorsod->encrypt(TestEnvironment::_buffer,
				      &header,
				      TestEnvironment::_source);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  std::string source(TestEnvironment::_buffer);
  TestEnvironment::_buffer.clear();
  std::string_view encrypted1;
  if (CryptoPlPlPtr encryptorpp = cryptoWeakC1.lock())
  encrypted1 = encryptorpp->encrypt(TestEnvironment::_buffer,
				    nullptr,
				    source);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted1));
  std::string data(encrypted1);
  TestEnvironment::_buffer.clear();
  if (CryptoPlPlPtr encryptorpp = cryptoWeakS1.lock())
  encryptorpp->decrypt(TestEnvironment::_buffer, data);
  ASSERT_TRUE(CryptoBase::isEncrypted(data));
  TestEnvironment::_buffer.clear();
  if (CryptoSodiumPtr encryptorsod = cryptoWeakS0.lock())
  encryptorsod->decrypt(TestEnvironment::_buffer, data);
  ASSERT_FALSE(CryptoBase::isEncrypted(data));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, data.data());
  ASSERT_EQ(header, recoveredHeader);
  std::string_view payload(data.cbegin() + HEADER_SIZE, data.cend());
  ASSERT_EQ(payload, TestEnvironment::_source);
}
