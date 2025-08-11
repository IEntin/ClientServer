/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoPlPl.h"

#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=TestCompressEncrypt*; done
// for i in {1..10}; do ./testbin --gtest_filter=AuthenticationTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=Base64EncodingTest*; done

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_LZ4_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::LZ4);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_SNAPPY_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::SNAPPY);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_ZSTD_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::ZSTD);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_NONE_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::NONE);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_LZ4_P) {
  Options::_doEncrypt = false;
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::LZ4);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_SNAPPY_P) {
  Options::_doEncrypt = false;
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::SNAPPY);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_ZSTD_P) {
  Options::_doEncrypt = false;
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::ZSTD);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_NONE_P) {
  Options::_doEncrypt = false;
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::NONE);
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
