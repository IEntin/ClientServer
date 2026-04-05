/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=DoubleEncryptDecrypt*; done

TEST(DoubleEncryptDecrypt, 0) {
  CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();
  CryptoTuple serverTuple = cryptotuple::getServerEncryptorTuple();

  std::string source = TestEnvironment::_source;

  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  std::string encrypted;
  CryptoWeakSodiumPtr cryptoWeakSodiumPtrClient = std::get<CryptoWeakSodiumPtr>(clientTuple);
  if (CryptoSodiumPtr encryptorClientSodium = cryptoWeakSodiumPtrClient.lock()) {
    TestEnvironment::_buffer.clear();
    encrypted = encryptorClientSodium->encrypt(TestEnvironment::_buffer,
					       &header,
					       source);
  }
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  CryptoWeakPlPlPtr cryptoWeakPlPlPtrClient = std::get<CryptoWeakPlPlPtr>(clientTuple);
  if (CryptoPlPlPtr encryptorClientPlPl = cryptoWeakPlPlPtrClient.lock()) {
    TestEnvironment::_buffer.clear();
    encrypted = encryptorClientPlPl->encrypt(TestEnvironment::_buffer,
					     nullptr,
					     encrypted);
  }
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  std::string decrypted;
  CryptoWeakPlPlPtr cryptoWeakPlPlPtrServer = std::get<CryptoWeakPlPlPtr>(serverTuple);
  if (CryptoPlPlPtr encryptorServerPlPlPtr = cryptoWeakPlPlPtrServer.lock()) {
    TestEnvironment::_buffer.clear();
    encryptorServerPlPlPtr->decrypt(TestEnvironment::_buffer, encrypted);
    decrypted = TestEnvironment::_buffer;
  }
  ASSERT_TRUE(CryptoBase::isEncrypted(decrypted));
  CryptoWeakSodiumPtr cryptoWeakSodiumPtrServer = std::get<CryptoWeakSodiumPtr>(serverTuple);
  if (CryptoSodiumPtr encryptorServerSodiumPtr = cryptoWeakSodiumPtrServer.lock()) {
    TestEnvironment::_buffer.clear();
    encryptorServerSodiumPtr->decrypt(TestEnvironment::_buffer, decrypted);
    decrypted = TestEnvironment::_buffer;
  }
  ASSERT_FALSE(CryptoBase::isEncrypted(decrypted));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, decrypted.data());
  ASSERT_EQ(header, recoveredHeader);
  std::string_view recoveredSource(decrypted.cbegin() + HEADER_SIZE, decrypted.cend());
  ASSERT_EQ(TestEnvironment::_source, recoveredSource);
}
