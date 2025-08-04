/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoPlPl.h"

#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=TestCompressEncrypt*; done
// for i in {1..10}; do ./testbin --gtest_filter=AuthenticationTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=Base64EncodingTest*; done

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_LZ4_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::ENCRYPT, COMPRESSORS::LZ4);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_SNAPPY_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::ENCRYPT, COMPRESSORS::SNAPPY);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_ZSTD_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::ENCRYPT, COMPRESSORS::ZSTD);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_NONE_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::ENCRYPT, COMPRESSORS::NONE);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_LZ4_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::NONE, COMPRESSORS::LZ4);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_SNAPPY_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::NONE, COMPRESSORS::SNAPPY);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_ZSTD_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::NONE, COMPRESSORS::ZSTD);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_NONE_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::NONE, COMPRESSORS::NONE);
}

TEST(AuthenticationTest, 1) {
  try {
    // client
    CryptoPlPlPtr cryptoC(std::make_shared<CryptoPlPl>(utility::generateRawUUID()));
    // server ctor throws on authentication failure
    CryptoPlPlPtr cryptoS = createServer(cryptoC);
  }
  catch (...) {
    // no exceptions
    ASSERT_TRUE(false);
  }
}

TEST(Base64EncodingTest, 1) {
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  CryptoPP::AutoSeededRandomPool prng;
  prng.GenerateBlock(key, key.size());
  CryptoPlPl crypto(utility::generateRawUUID());
  std::string encoded = crypto.base64_encode(key);
  std::vector<unsigned char> vect = crypto.base64_decode(encoded);
  CryptoPP::SecByteBlock recovered(vect.data(), vect.size());
  ASSERT_EQ(key, recovered);
}
