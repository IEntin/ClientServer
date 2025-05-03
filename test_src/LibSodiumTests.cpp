
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <sodium.h>

#include "ClientOptions.h"
#include "CryptoSodium.h"
#include "Header.h"
#include "TestEnvironment.h"
#include "Utility.h"

// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.encryption; done
// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.authentication; done
// for i in {1..10}; do ./testbin --gtest_filter=CompressEncryptSodiumTest*; done

TEST(LibSodiumTest, authentication) {
  ASSERT_FALSE(sodium_init() < 0);
  CryptoSodium crypto(utility::generateRawUUID());
  const auto& hashed = crypto.getMsgHash();
  const auto& pubcicKeySign = crypto.getPublicKeySign();
  const auto& signature = crypto.getSignature();
  ASSERT_TRUE(crypto_sign_verify_detached(signature.data(), hashed.data(), hashed.size(), pubcicKeySign.data()) == 0);
}

TEST(LibSodiumTest, hashing) {
  ASSERT_FALSE(sodium_init() < 0);
  CryptoSodium crypto(utility::generateRawUUID());
  const auto& hashed(crypto.getMsgHash());
  ASSERT_EQ(hashed.size(), crypto_generichash_BYTES);
  std::cout << "generichash:";
  for (auto element : hashed)
    std::cout << std::hex << std::setw(2) << std::setfill('0')
	      << static_cast<int>(element);
  std::cout << '\n';
}

TEST(LibSodiumTest, DHkeyExchange) {
  ASSERT_FALSE(sodium_init() < 0);
  unsigned char client_pk[crypto_kx_PUBLICKEYBYTES];
  unsigned char client_sk[crypto_kx_SECRETKEYBYTES];
  unsigned char server_pk[crypto_kx_PUBLICKEYBYTES];
  unsigned char server_sk[crypto_kx_SECRETKEYBYTES];
  unsigned char client_rx[crypto_kx_SESSIONKEYBYTES];
  unsigned char client_tx[crypto_kx_SESSIONKEYBYTES];
  unsigned char server_rx[crypto_kx_SESSIONKEYBYTES];
  unsigned char server_tx[crypto_kx_SESSIONKEYBYTES];
  // Generate key pairs for client and server
  crypto_kx_keypair(client_pk, client_sk);
  crypto_kx_keypair(server_pk, server_sk);
  // Client-side key exchange
  ASSERT_TRUE(crypto_kx_client_session_keys(client_rx, client_tx, client_pk, client_sk, server_pk) == 0);
  // Server-side key exchange
  ASSERT_TRUE(crypto_kx_server_session_keys(server_rx, server_tx, server_pk, server_sk, client_pk) == 0);
  // Verify that the shared secrets match
  ASSERT_TRUE(sodium_memcmp(client_rx, server_tx, crypto_kx_SESSIONKEYBYTES) == 0);
  ASSERT_TRUE(sodium_memcmp(client_tx, server_rx, crypto_kx_SESSIONKEYBYTES) == 0);
}

TEST(LibSodiumTest, encryption) {
  ASSERT_FALSE(sodium_init() < 0);
  ASSERT_TRUE(crypto_aead_aes256gcm_is_available());
  ASSERT_FALSE(sodium_init() < 0);
  HEADER header{ HEADERTYPE::SESSION, 0, HEADER_SIZE + TestEnvironment::_source.size(), ClientOptions::_encryption,
		 COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
  CryptoSodium crypto(utility::generateRawUUID());
  crypto.setDummyAesKey();
  std::string_view encrypted = crypto.encrypt(TestEnvironment::_buffer,
					      header,
					      TestEnvironment::_source);
  ASSERT_TRUE(utility::isEncrypted(encrypted));
  std::string data(encrypted);
  crypto.decrypt(TestEnvironment::_buffer, data);
  ASSERT_FALSE(utility::isEncrypted(data));
  HEADER recoveredHeader;
  deserialize(recoveredHeader, data.data());
  ASSERT_EQ(header, recoveredHeader);
  ASSERT_TRUE(data.erase(0, HEADER_SIZE) == TestEnvironment::_source);
}

TEST(LibSodiumTest, publicKeyEncoding) {
  ASSERT_FALSE(sodium_init() < 0);
  CryptoSodium crypto(utility::generateRawUUID());
  unsigned char public_key[crypto_box_PUBLICKEYBYTES];
  unsigned char secret_key[crypto_box_SECRETKEYBYTES];
  crypto_box_keypair(public_key, secret_key);
  std::vector<unsigned char> original_data(public_key, public_key + crypto_box_PUBLICKEYBYTES);
  std::string encoded = crypto.base64_encode(original_data);
  std::vector<unsigned char> decoded_data = crypto.base64_decode(encoded);
  ASSERT_EQ(original_data, decoded_data);
}

struct CompressEncryptSodiumTest : testing::Test {
  void testCompressEncrypt(CRYPTO cryptoType, COMPRESSORS compressor) {
    auto crypto(std::make_shared<CryptoSodium>(utility::generateRawUUID()));
    crypto->setDummyAesKey();
    // must be a copy
    std::string data = TestEnvironment::_source;
    HEADER header{ HEADERTYPE::SESSION,
		   0,
		   data.size(),
		   cryptoType,
		   compressor,
		   DIAGNOSTICS::NONE,
		   STATUS::NONE,
		   0 };
    printHeader(header, LOG_LEVEL::ALWAYS);
    std::string_view dataView =
      utility::compressEncrypt(TestEnvironment::_buffer, header, std::weak_ptr(crypto), data);
    ASSERT_EQ(utility::isEncrypted(data), doEncrypt(header));
    HEADER restoredHeader;
    data = dataView;
    utility::decryptDecompress(TestEnvironment::_buffer, restoredHeader, std::weak_ptr(crypto), data);
    ASSERT_EQ(header, restoredHeader);
    ASSERT_EQ(data, TestEnvironment::_source);
  }
  void TearDown() {}
};

TEST_F(CompressEncryptSodiumTest, ENCRYPT_COMPRESSORS_LZ4) {
  testCompressEncrypt(CRYPTO::CRYPTOSODIUM, COMPRESSORS::LZ4);
}

TEST_F(CompressEncryptSodiumTest, ENCRYPT_COMPRESSORS_SNAPPY) {
  testCompressEncrypt(CRYPTO::CRYPTOSODIUM, COMPRESSORS::SNAPPY);
}

TEST_F(CompressEncryptSodiumTest, ENCRYPT_COMPRESSORS_ZSTD) {
  testCompressEncrypt(CRYPTO::CRYPTOSODIUM, COMPRESSORS::ZSTD);
}

TEST_F(CompressEncryptSodiumTest, ENCRYPT_COMPRESSORS_NONE) {
  testCompressEncrypt(CRYPTO::CRYPTOSODIUM, COMPRESSORS::NONE);
}

TEST_F(CompressEncryptSodiumTest, NOTENCRYPT_COMPRESSORS_LZ4) {
  testCompressEncrypt(CRYPTO::NONE, COMPRESSORS::LZ4);
}

TEST_F(CompressEncryptSodiumTest, NOTENCRYPT_COMPRESSORS_SNAPPY) {
  testCompressEncrypt(CRYPTO::NONE, COMPRESSORS::SNAPPY);
}

TEST_F(CompressEncryptSodiumTest, NOTENCRYPT_COMPRESSORS_ZSTD) {
  testCompressEncrypt(CRYPTO::NONE, COMPRESSORS::ZSTD);
}

TEST_F(CompressEncryptSodiumTest, NOTENCRYPT_COMPRESSORS_NONE) {
  testCompressEncrypt(CRYPTO::NONE, COMPRESSORS::NONE);
}
