
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.authentication; done
// for i in {1..10}; do ./testbin --gtest_filter=TestCompressEncrypt*; done
// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.DHkeyExchange; done

TEST(LibSodiumTest, authentication) {
  try {
    // client
    CryptoSodiumPtr cryptoC(std::make_shared<CryptoSodium>());
    // server
    // server ctor throws on authentication failure
    CryptoSodiumPtr cryptoS = createServerEncryptor(cryptoC);
  }
  catch (...) {
    // no exceptions
    ASSERT_TRUE(false);
  }
}

TEST(LibSodiumTest, DHkeyExchange) {
  try {
    // client
    CryptoSodiumPtr cryptoC(std::make_shared<CryptoSodium>());
    // server
    CryptoSodiumPtr cryptoS = createServerEncryptor(cryptoC);
    cryptoC->clientKeyExchange(cryptoS->_encodedPubKeyAes);
    // test encrypt - decrypt
    HEADER header{ HEADERTYPE::SESSION, 0, TestEnvironment::_source.size(),
		   COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
    std::string_view encrypted = cryptoC->encrypt(TestEnvironment::_buffer,
						  TestEnvironment::_source,
						  &header);
    ASSERT_TRUE(CryptoBase::isEncrypted(encrypted));
    std::string data(encrypted);
    cryptoS->decrypt(TestEnvironment::_buffer, data);
    ASSERT_FALSE(CryptoBase::isEncrypted(data));
    HEADER recoveredHeader;
    deserialize(recoveredHeader, data.data());
    ASSERT_EQ(header, recoveredHeader);
    std::string_view payload(data.cbegin() + HEADER_SIZE, data.cend());
    ASSERT_EQ(payload, TestEnvironment::_source);
  }
  catch (...) {
    // no exceptions
    ASSERT_TRUE(false);
  }
}

TEST(LibSodiumTest, publicKeyEncoding) {
  unsigned char public_key[crypto_box_PUBLICKEYBYTES];
  unsigned char secret_key[crypto_box_SECRETKEYBYTES];
  crypto_box_keypair(public_key, secret_key);
  std::vector<unsigned char> original_data = { std::cbegin(public_key), std::cend(public_key) };
  CryptoSodium crypto;
  std::string encoded = crypto.base64_encode(original_data);
  ASSERT_EQ(original_data, crypto.base64_decode(encoded));
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_LZ4_S) {
  testCompressEncrypt(COMPRESSORS::LZ4, CRYPTO::CRYPTOSODIUM, true);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_SNAPPY_S) {
  testCompressEncrypt(COMPRESSORS::SNAPPY, CRYPTO::CRYPTOSODIUM, true);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_ZSTD_S) {
  testCompressEncrypt(COMPRESSORS::ZSTD, CRYPTO::CRYPTOSODIUM, true);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_NONE_S) {
  testCompressEncrypt(COMPRESSORS::NONE, CRYPTO::CRYPTOSODIUM, true);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_LZ4_S) {
  testCompressEncrypt(COMPRESSORS::LZ4, CRYPTO::CRYPTOSODIUM, false);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_SNAPPY_S) {
  testCompressEncrypt(COMPRESSORS::SNAPPY, CRYPTO::CRYPTOSODIUM, false);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_ZSTD_S) {
  testCompressEncrypt(COMPRESSORS::ZSTD, CRYPTO::CRYPTOSODIUM, false);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_NONE_S) {
  testCompressEncrypt(COMPRESSORS::NONE, CRYPTO::CRYPTOSODIUM, false);
}
