
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "DebugLog.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.authentication; done
// for i in {1..10}; do ./testbin --gtest_filter=TestCompressEncrypt*; done
// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.DHkeyExchange; done

TEST(LibSodiumTest, authentication) {
  DebugLog::setTitle("LibSodiumTest, authentication");
  try {
    // client
    CryptoSodiumPtr cryptoC(std::make_shared<CryptoSodium>(utility::generateRawUUID()));
    // server
    // server ctor throws on authentication failure
    CryptoSodiumPtr cryptoS = createServer(cryptoC);
  }
  catch (...) {
    // no exceptions
    ASSERT_TRUE(false);
  }
}

TEST(LibSodiumTest, DHkeyExchange) {
  DebugLog::setTitle("LibSodiumTest, DHkeyExchange");
  try {
    // client
    CryptoSodiumPtr cryptoC(std::make_shared<CryptoSodium>(utility::generateRawUUID()));
    // server
    CryptoSodiumPtr cryptoS = createServer(cryptoC);
    cryptoC->clientKeyExchange(cryptoS->_encodedPubKeyAes);
    // test encrypt - decrypt
    HEADER header{ HEADERTYPE::SESSION, 0, HEADER_SIZE + std::ssize(TestEnvironment::_source), CRYPTO::CRYPTOSODIUM,
		   COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
    std::string_view encrypted = cryptoC->encrypt(TestEnvironment::_buffer,
						  header,
						  TestEnvironment::_source);
    ASSERT_TRUE(utility::isEncrypted(encrypted));
    std::string data(encrypted);
    cryptoS->decrypt(TestEnvironment::_buffer, data);
    ASSERT_FALSE(utility::isEncrypted(data));
    HEADER recoveredHeader;
    deserialize(recoveredHeader, data.data());
    ASSERT_EQ(header, recoveredHeader);
    ASSERT_TRUE(data.erase(0, HEADER_SIZE) == TestEnvironment::_source);
  }
  catch (...) {
    // no exceptions
    ASSERT_TRUE(false);
  }
}

TEST(LibSodiumTest, publicKeyEncoding) {
  DebugLog::setTitle("LibSodiumTest, publicKeyEncoding");
  unsigned char public_key[crypto_box_PUBLICKEYBYTES];
  unsigned char secret_key[crypto_box_SECRETKEYBYTES];
  crypto_box_keypair(public_key, secret_key);
  std::vector<unsigned char> original_data = { std::cbegin(public_key), std::cend(public_key) };
  CryptoSodium crypto(utility::generateRawUUID());
  std::string encoded = crypto.base64_encode(original_data);
  ASSERT_EQ(original_data, crypto.base64_decode(encoded));
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_LZ4_S) {
  testCompressEncrypt<CryptoSodium>(CRYPTO::CRYPTOSODIUM, COMPRESSORS::LZ4);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_SNAPPY_S) {
  testCompressEncrypt<CryptoSodium>(CRYPTO::CRYPTOSODIUM, COMPRESSORS::SNAPPY);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_ZSTD_S) {
  testCompressEncrypt<CryptoSodium>(CRYPTO::CRYPTOSODIUM, COMPRESSORS::ZSTD);
}

TEST_F(TestCompressEncrypt, ENCRYPT_COMPRESSORS_NONE_S) {
  testCompressEncrypt<CryptoSodium>(CRYPTO::CRYPTOSODIUM, COMPRESSORS::NONE);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_LZ4_S) {
  Options::_doEncrypt = false;
  testCompressEncrypt<CryptoSodium>(CRYPTO::CRYPTOSODIUM, COMPRESSORS::LZ4);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_SNAPPY_S) {
  Options::_doEncrypt = false;
  testCompressEncrypt<CryptoSodium>(CRYPTO::CRYPTOSODIUM, COMPRESSORS::SNAPPY);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_ZSTD_S) {
  Options::_doEncrypt = false;
  testCompressEncrypt<CryptoSodium>(CRYPTO::CRYPTOSODIUM, COMPRESSORS::ZSTD);
}

TEST_F(TestCompressEncrypt, NOTENCRYPT_COMPRESSORS_NONE_S) {
  Options::_doEncrypt = false;
  testCompressEncrypt<CryptoSodium>(CRYPTO::CRYPTOSODIUM, COMPRESSORS::NONE);
}
