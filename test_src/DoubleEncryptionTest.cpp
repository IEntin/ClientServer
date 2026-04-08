/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=DoubleEncryptDecrypt*; done

TEST(DoubleEncryptDecrypt, 0) {
  CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();

  std::string source = TestEnvironment::_source;

  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  std::string encrypted = cryptotuple::doubleEncrypt(clientTuple,
						     TestEnvironment:: _buffer,
						     header,
						     source);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  CryptoTuple serverTuple = cryptotuple::getServerEncryptorTuple();

  std::string decrypted = cryptotuple::doubleDecrypt(serverTuple,
						     TestEnvironment::_buffer,
						     header,
						     encrypted);
  ASSERT_FALSE(CryptoBase::isEncrypted(decrypted));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, decrypted.data());
  ASSERT_EQ(header, recoveredHeader);
  std::string_view recoveredSource(decrypted.cbegin() + HEADER_SIZE, decrypted.cend());
  ASSERT_EQ(TestEnvironment::_source, recoveredSource);
}
