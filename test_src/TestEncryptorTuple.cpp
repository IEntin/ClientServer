/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=DoubleEncryptDecrypt*; done

TEST(RuntimeTupleAccess, 1) {
  auto encryptors = std::make_tuple(std::make_shared<CryptoSodium>(), std::make_shared<CryptoPlPl>());
  auto cryptopp = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOPP)>(encryptors);
  ASSERT_TRUE(cryptopp->getName() == "CryptoPlPl");
  auto cryptosodium = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOSODIUM)>(encryptors);
  ASSERT_TRUE(cryptosodium->getName() == "CryptoSodium");
}

TEST(EncryptDecrypt, 0) {
  CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();
  CryptoSodiumPtr cryptoC0 = std::get<0>(clientTuple);
  CryptoTuple serverTuple = cryptotuple::getServerEncryptorTuple();
  CryptoSodiumPtr cryptoS0 = std::get<0>(serverTuple);
  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  std::string_view encrypted = cryptoC0->encrypt(TestEnvironment::_buffer,
						 TestEnvironment::_source,
						 &header);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  std::string data(encrypted);
  cryptoS0->decrypt(TestEnvironment::_buffer, data);
  ASSERT_FALSE(CryptoBase::isEncrypted(data));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, data.data());
  ASSERT_EQ(header, recoveredHeader);
  std::string_view payload(data.cbegin() + HEADER_SIZE, data.cend());
  ASSERT_EQ(payload, TestEnvironment::_source);
}

TEST(EncryptDecrypt, 1) {
  CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();
  CryptoPlPlPtr cryptoC1 = std::get<1>(clientTuple);
  CryptoTuple serverTuple = cryptotuple::getServerEncryptorTuple();
  CryptoPlPlPtr cryptoS1 = std::get<1>(serverTuple);
  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  std::string_view encrypted = cryptoC1->encrypt(TestEnvironment::_buffer,
						 TestEnvironment::_source,
						 &header);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  std::string data(encrypted);
  cryptoS1->decrypt(TestEnvironment::_buffer, data);
  ASSERT_FALSE(CryptoBase::isEncrypted(data));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, data.data());
  ASSERT_EQ(header, recoveredHeader);
  std::string_view payload(data.cbegin() + HEADER_SIZE, data.cend());
  ASSERT_EQ(payload, TestEnvironment::_source);
}

TEST(DoubleEncryptDecrypt, 0) {
  CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();
  CryptoSodiumPtr cryptoC0 = std::get<0>(clientTuple);
  CryptoPlPlPtr cryptoC1 = std::get<1>(clientTuple);
  
  CryptoTuple serverTuple = cryptotuple::getServerEncryptorTuple();
  CryptoSodiumPtr cryptoS0 = std::get<0>(serverTuple);
  CryptoPlPlPtr cryptoS1 = std::get<1>(serverTuple);

  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  std::string_view encrypted = cryptoC0->encrypt(TestEnvironment::_buffer,
						 TestEnvironment::_source,
						 &header);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  std::string source(TestEnvironment::_buffer);
  TestEnvironment::_buffer.clear();
  std::string_view encrypted1 = cryptoC1->encrypt(TestEnvironment::_buffer,
						  source);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted1));
  std::string data(encrypted1);
  TestEnvironment::_buffer.clear();
  cryptoS1->decrypt(TestEnvironment::_buffer, data);
  ASSERT_TRUE(CryptoBase::isEncrypted(data));
  TestEnvironment::_buffer.clear();
  cryptoS0->decrypt(TestEnvironment::_buffer, data);
  ASSERT_FALSE(CryptoBase::isEncrypted(data));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, data.data());
  ASSERT_EQ(header, recoveredHeader);
  std::string_view payload(data.cbegin() + HEADER_SIZE, data.cend());
  ASSERT_EQ(payload, TestEnvironment::_source);
}

TEST(DoubleEncryptDecrypt, 1) {
  CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();
  CryptoSodiumPtr cryptoC0 = std::get<0>(clientTuple);
  CryptoPlPlPtr cryptoC1 = std::get<1>(clientTuple);
  
  CryptoTuple serverTuple = cryptotuple::getServerEncryptorTuple();
  CryptoSodiumPtr cryptoS0 = std::get<0>(serverTuple);
  CryptoPlPlPtr cryptoS1 = std::get<1>(serverTuple);

  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  std::string_view encrypted = cryptoC1->encrypt(TestEnvironment::_buffer,
						 TestEnvironment::_source,
						 &header);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  std::string source(TestEnvironment::_buffer);
  TestEnvironment::_buffer.clear();
  std::string_view encrypted1 = cryptoC0->encrypt(TestEnvironment::_buffer,
						  source);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted1));
  std::string data(encrypted1);
  TestEnvironment::_buffer.clear();
  cryptoS0->decrypt(TestEnvironment::_buffer, data);
  ASSERT_TRUE(CryptoBase::isEncrypted(data));
  TestEnvironment::_buffer.clear();
  cryptoS1->decrypt(TestEnvironment::_buffer, data);
  ASSERT_FALSE(CryptoBase::isEncrypted(data));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, data.data());
  ASSERT_EQ(header, recoveredHeader);
  std::string_view payload(data.cbegin() + HEADER_SIZE, data.cend());
  ASSERT_EQ(payload, TestEnvironment::_source);
}
