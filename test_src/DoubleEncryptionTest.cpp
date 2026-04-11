/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoTuple.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=DoubleEncryptDecrypt*; done
// for i in {1..10}; do ./testbin --gtest_filter=TestCompressDoubleEncrypt*;done

TEST(DoubleEncryptDecrypt, 0) {
  const CryptoTuple& clientTuple = cryptotuple::getClientEncryptorTuple();

  std::string source = TestEnvironment::_source;

  HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  std::string encrypted = cryptotuple::doubleEncrypt(clientTuple,
						     TestEnvironment:: _buffer,
						     header,
						     source);
  ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
  const CryptoTuple& serverTuple = cryptotuple::getServerEncryptorTuple();

  cryptotuple::doubleDecrypt(serverTuple,
			     TestEnvironment::_buffer,
			     header,
			     encrypted);
  std::string decrypted = TestEnvironment::_buffer;
  ASSERT_FALSE(CryptoBase::isEncrypted(decrypted));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, decrypted.data());
  ASSERT_EQ(header, recoveredHeader);
  decrypted.erase(0, HEADER_SIZE);
  ASSERT_EQ(TestEnvironment::_source, decrypted);
}

struct TestCompressDoubleEncrypt : testing::Test {
  static void testCompressDoubleEncrypt(COMPRESSORS compressor,
					bool doEncrypt) {
    std::string data = TestEnvironment::_source;
    HEADER header{ HEADERTYPE::SESSION,
		   0,
		   data.size(),
		   compressor,
		   DIAGNOSTICS::NONE,
		   STATUS::NONE,
		   0 };
    CryptoTuple clientTuple = cryptotuple::getClientEncryptorTuple();
    std::string encrypted = encryptortemplates::compressDoubleEncrypt(clientTuple,
								      TestEnvironment::_buffer,
								      header,
								      data,
								      doEncrypt);
    ASSERT_EQ(CryptoBase::isEncrypted(encrypted), doEncrypt);
    const CryptoTuple& serverTuple = cryptotuple::getServerEncryptorTuple();
    TestEnvironment::_buffer.clear();
    HEADER recoveredHeader;
    encryptortemplates::doubleDecryptDecompress(serverTuple,
						TestEnvironment::_buffer,
						recoveredHeader,
						encrypted);
    ASSERT_EQ(header, recoveredHeader);
    ASSERT_EQ(encrypted, TestEnvironment::_source);
  }
  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(TestCompressDoubleEncrypt, ENCRYPT_COMPRESSORS_LZ4) {
  testCompressDoubleEncrypt(COMPRESSORS::LZ4, true);
}

TEST_F(TestCompressDoubleEncrypt, ENCRYPT_COMPRESSORS_ZSTD) {
  testCompressDoubleEncrypt(COMPRESSORS::ZSTD, true);
}

TEST_F(TestCompressDoubleEncrypt, NOTENCRYPT_COMPRESSORS_ZSTD) {
  testCompressDoubleEncrypt(COMPRESSORS::ZSTD, false);
}

TEST_F(TestCompressDoubleEncrypt, ENCRYPT_COMPRESSORS_SNAPPY) {
  testCompressDoubleEncrypt(COMPRESSORS::SNAPPY, true);
}
