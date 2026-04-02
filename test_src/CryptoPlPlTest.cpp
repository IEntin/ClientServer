/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=TestCompressEncrypt*; done
// for i in {1..10}; do ./testbin --gtest_filter=AuthenticationTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=Base64EncodingTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=VariantCrypto*; done

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_LZ4_P) {
  testCompressEncrypt(COMPRESSORS::LZ4, true, CRYPTO::CRYPTOPP);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_SNAPPY_P) {
  testCompressEncrypt(COMPRESSORS::SNAPPY, true, CRYPTO::CRYPTOPP);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_ZSTD_P) {
  testCompressEncrypt(COMPRESSORS::ZSTD, true, CRYPTO::CRYPTOPP);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_NONE_P) {
  testCompressEncrypt(COMPRESSORS::NONE, true, CRYPTO::CRYPTOPP);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_LZ4_P) {
  testCompressEncrypt(COMPRESSORS::LZ4, false, CRYPTO::CRYPTOPP);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_SNAPPY_P) {
  testCompressEncrypt(COMPRESSORS::SNAPPY, false, CRYPTO::CRYPTOPP);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_ZSTD_P) {
  testCompressEncrypt(COMPRESSORS::ZSTD, false, CRYPTO::CRYPTOPP);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_NONE_P) {
  testCompressEncrypt(COMPRESSORS::NONE, false, CRYPTO::CRYPTOPP);
  TestEnvironment::reset();
}

TEST(AuthenticationTest, 1) {
  try {
    // client
    CryptoPlPlPtr cryptoC(std::make_shared<CryptoPlPl>());
    // server ctor throws on authentication failure
    CryptoPlPlPtr cryptoS = createServerEncryptor(cryptoC);
  }
  catch (const std::exception& e) {
    LogError<< e.what() << '\n';
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

TEST(CryptoVariant, 1) {
  CryptoVariant cryptoVariant0 = std::make_shared<CryptoPlPl>();
  std::size_t index = cryptoVariant0.index();
  ASSERT_EQ(index, 1);
  CryptoPlPlPtr active0 = std::get<CryptoPlPlPtr>(cryptoVariant0);
  static_assert(std::is_same_v<decltype(active0), CryptoPlPlPtr> == true );
  CryptoVariant cryptoVariant1 = std::make_shared<CryptoSodium>();
  index = cryptoVariant1.index();
  ASSERT_EQ(index, 0);
  CryptoSodiumPtr active2 = std::get<CryptoSodiumPtr>(cryptoVariant1);
  static_assert(std::is_same_v<decltype(active2), CryptoSodiumPtr> == true );
 }
