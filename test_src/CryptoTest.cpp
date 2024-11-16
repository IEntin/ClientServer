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
  auto crypto(std::make_shared<CryptoNS>());
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  CryptoPP::AutoSeededRandomPool prng;
  prng.GenerateBlock(key, key.size());
  std::string_view data = TestEnvironment::_source;
  std::string_view cipher = crypto->encrypt(true, data);
  std::string_view decrypted = crypto->decrypt(cipher);
  ASSERT_EQ(data, decrypted);
  std::string_view cipher2 = crypto->encrypt(false, data);
  std::string_view decrypted2 = crypto->decrypt(cipher2);
  ASSERT_EQ(data, decrypted2);
}

struct CompressEncryptTest : testing::Test {
  void testCompressEncrypt(bool encrypt, COMPRESSORS compressor) {
    auto crypto(std::make_shared<CryptoNS>());
    CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::AutoSeededRandomPool prng;
    prng.GenerateBlock(key, key.size());
    // must be a copy
    std::string data = TestEnvironment::_source;
    HEADER header{ HEADERTYPE::SESSION,
		   data.size(),
		   compressor,
		   DIAGNOSTICS::NONE,
		   STATUS::NONE,
		   0 };
    std::string_view transformed[[maybe_unused]] = utility::compressEncryptNS(encrypt, header, crypto, data);
    HEADER restoredHeader;    
    std::string_view restored = utility::decryptDecompressNS(restoredHeader, crypto, data);
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
