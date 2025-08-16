/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=TestCompressEncrypt*; done
// for i in {1..10}; do ./testbin --gtest_filter=AuthenticationTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=Base64EncodingTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=VariantCrypto*; done

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_LZ4_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::LZ4, true);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_SNAPPY_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::SNAPPY, true);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_ZSTD_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::ZSTD, true);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_NONE_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::NONE, true);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_LZ4_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::LZ4, false);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_SNAPPY_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::SNAPPY, false);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_ZSTD_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::ZSTD, false);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_NONE_P) {
  testCompressEncrypt<CryptoPlPl>(CRYPTO::CRYPTOPP, COMPRESSORS::NONE, false);
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

TEST(VariantCrypto, 1) {
  Options::_encryption = CRYPTO::CRYPTOPP;
  constexpr unsigned long expected1 = utility::getEncryptionIndex(CRYPTO::CRYPTOPP);
  std::variant<CryptoPlPlPtr, CryptoSodiumPtr> cryptoVariant = utility::createCrypto(utility::generateRawUUID());
  std::size_t index = cryptoVariant.index();
  ASSERT_EQ(index, expected1);
  auto held1 = std::get<expected1>(cryptoVariant);
  static_assert(std::is_same_v<decltype(held1), CryptoPlPlPtr> == true );
  Options::_encryption = CRYPTO::CRYPTOSODIUM;
  constexpr unsigned long expected2 = utility::getEncryptionIndex(CRYPTO::CRYPTOSODIUM);
  cryptoVariant = utility::createCrypto(utility::generateRawUUID());
  index = cryptoVariant.index();
  ASSERT_EQ(index, expected2);
  auto held2 = std::get<expected2>(cryptoVariant);
  static_assert(std::is_same_v<decltype(held2), CryptoSodiumPtr> == true );
  TestEnvironment::reset();
}
