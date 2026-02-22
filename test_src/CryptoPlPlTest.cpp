/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=TestCompressEncrypt*; done
// for i in {1..10}; do ./testbin --gtest_filter=AuthenticationTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=Base64EncodingTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=VariantCrypto*; done


TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_LZ4_P) {
  if (CRYPTO::CRYPTOPP == Options::_encryptorTypeDefault) {
    testCompressEncrypt(COMPRESSORS::LZ4, true);
  }
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_SNAPPY_P) {
  if (CRYPTO::CRYPTOPP == Options::_encryptorTypeDefault) {
    testCompressEncrypt(COMPRESSORS::SNAPPY, true);
  }
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_ZSTD_P) {
  if (CRYPTO::CRYPTOPP == Options::_encryptorTypeDefault) {
    testCompressEncrypt(COMPRESSORS::ZSTD, true);
  }
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_NONE_P) {
  if (CRYPTO::CRYPTOPP == Options::_encryptorTypeDefault) {
    testCompressEncrypt(COMPRESSORS::NONE, true);
  }
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_LZ4_P) {
  if (CRYPTO::CRYPTOPP == Options::_encryptorTypeDefault) {
    testCompressEncrypt(COMPRESSORS::LZ4, false);
  }
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_SNAPPY_P) {
  if (CRYPTO::CRYPTOPP == Options::_encryptorTypeDefault) {
    testCompressEncrypt(COMPRESSORS::SNAPPY, false);
  }
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_ZSTD_P) {
  if (CRYPTO::CRYPTOPP == Options::_encryptorTypeDefault) {
    testCompressEncrypt(COMPRESSORS::ZSTD, false);
  }
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_NONE_P) {
  if (CRYPTO::CRYPTOPP == Options::_encryptorTypeDefault) {
    testCompressEncrypt(COMPRESSORS::NONE, false);
  }
}

TEST(AuthenticationTest, 1) {
  try {
    // client
    CryptoPlPlPtr cryptoC(std::make_shared<CryptoPlPl>());
    // server ctor throws on authentication failure
    CryptoPlPlPtr cryptoS = createServerEncryptor(cryptoC);
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

TEST(CryptoVariant, 1) {
  std::variant<CryptoSodiumPtr, CryptoPlPlPtr> cryptoVariant0 = std::make_shared<CryptoPlPl>();
  std::size_t index = cryptoVariant0.index();
  ASSERT_EQ(index, 1);
  auto active0 = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOPP)>(cryptoVariant0);
  static_assert(std::is_same_v<decltype(active0), CryptoPlPlPtr> == true );
  std::variant<CryptoSodiumPtr, CryptoPlPlPtr> cryptoVariant1 = std::make_shared<CryptoSodium>();
  index = cryptoVariant1.index();
  ASSERT_EQ(index, 0);
  auto active2 = std::get<std::to_underlying<CRYPTO>(CRYPTO::CRYPTOSODIUM)>(cryptoVariant1);
  static_assert(std::is_same_v<decltype(active2), CryptoSodiumPtr> == true );
 }
