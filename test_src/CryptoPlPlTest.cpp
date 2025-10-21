/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=TestCompressEncrypt*; done
// for i in {1..10}; do ./testbin --gtest_filter=AuthenticationTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=Base64EncodingTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=VariantCrypto*; done

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_LZ4_P) {
  testCompressEncrypt<CryptoPlPl>(COMPRESSORS::LZ4, true);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_SNAPPY_P) {
  testCompressEncrypt<CryptoPlPl>(COMPRESSORS::SNAPPY, true);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_ZSTD_P) {
  testCompressEncrypt<CryptoPlPl>(COMPRESSORS::ZSTD, true);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_NONE_P) {
  testCompressEncrypt<CryptoPlPl>(COMPRESSORS::NONE, true);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_LZ4_P) {
  testCompressEncrypt<CryptoPlPl>(COMPRESSORS::LZ4, false);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_SNAPPY_P) {
  testCompressEncrypt<CryptoPlPl>(COMPRESSORS::SNAPPY, false);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_ZSTD_P) {
  testCompressEncrypt<CryptoPlPl>(COMPRESSORS::ZSTD, false);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_NONE_P) {
  testCompressEncrypt<CryptoPlPl>(COMPRESSORS::NONE, false);
}

TEST(AuthenticationTest, 1) {
  try {
    // client
    CryptoPlPlPtr cryptoC(std::make_shared<CryptoPlPl>());
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
  CryptoPlPl crypto;
  std::string encoded = crypto.base64_encode(key);
  std::vector<unsigned char> vect = crypto.base64_decode(encoded);
  CryptoPP::SecByteBlock recovered(vect.data(), vect.size());
  ASSERT_EQ(key, recovered);
}

TEST(VariantCrypto, 1) {
  std::variant<CryptoPlPlPtr, CryptoSodiumPtr> cryptoVariant;
  cryptodefinitions::createCrypto(CRYPTO::CRYPTOPP);
  cryptoVariant = cryptodefinitions::_encryptorVar;
  std::size_t index = cryptoVariant.index();
  ASSERT_EQ(index, 0);
  auto active1 = std::get<0>(cryptoVariant);
  static_assert(std::is_same_v<decltype(active1), CryptoPlPlPtr> == true );
  cryptodefinitions::createCrypto(CRYPTO::CRYPTOSODIUM);
  cryptoVariant = cryptodefinitions::_encryptorVar;
  index = cryptoVariant.index();
  ASSERT_EQ(index, 1);
  auto active2 = std::get<1>(cryptoVariant);
  static_assert(std::is_same_v<decltype(active2), CryptoSodiumPtr> == true );
  TestEnvironment::reset();
}
