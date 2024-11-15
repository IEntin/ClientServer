/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <cmath>

#include <boost/asio.hpp>

#include "TestEnvironment.h"
#include "Utility.h"

// for i in {1..10}; do ./testbin --gtest_filter=CryptoTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=CompressEncryptTest*; done

TEST(CryptoTest, 1) {
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  Crypto::_rng.GenerateBlock(key, key.size());
  std::string_view data = TestEnvironment::_source;
  std::string_view cipher = Crypto::encrypt(true, key, data);
  std::string ivStr1(cipher.end() - CryptoPP::AES::BLOCKSIZE, cipher.end());
  std::string ivStr2(cipher.end() - 2 * CryptoPP::AES::BLOCKSIZE, cipher.end() - CryptoPP::AES::BLOCKSIZE);
  std::string_view decrypted = Crypto::decrypt(key, cipher);
  ASSERT_EQ(data, decrypted);
}

struct CompressEncryptTest : testing::Test {
  void testCompressEncrypt(bool encrypt, COMPRESSORS compressor) {
    CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
    Crypto::_rng.GenerateBlock(key, key.size());
    // must be a copy
    std::string data = TestEnvironment::_source;
    HEADER header{ HEADERTYPE::SESSION,
		   data.size(),
		   compressor,
		   DIAGNOSTICS::NONE,
		   STATUS::NONE,
		   0 };
    std::string_view transformed[[maybe_unused]] = utility::compressEncrypt(encrypt, header, key, data);
    HEADER restoredHeader;    
    std::string_view restored = utility::decryptDecompress(restoredHeader, key, data);
    ASSERT_EQ(header, restoredHeader);
    ASSERT_EQ(restored, TestEnvironment::_source);
  }
  void TearDown() {}
};

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_LZ4) {
  testCompressEncrypt(true, COMPRESSORS::LZ4);
}

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_NONE) {
  testCompressEncrypt(true, COMPRESSORS::NONE);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_LZ4) {
  testCompressEncrypt(false, COMPRESSORS::LZ4);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_NONE) {
  testCompressEncrypt(false, COMPRESSORS::NONE);
}
