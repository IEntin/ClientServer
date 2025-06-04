
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
// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.DHkeyExchange; done

TEST(LibSodiumTest, authentication) {
  ASSERT_FALSE(sodium_init() < 0);
  // client
  CryptoSodium cryptoC(utility::generateRawUUID());
  std::span<const unsigned char> hashed = cryptoC.getMsgHash();
  std::span<const unsigned char> signatureWithPubKey = cryptoC.getSignatureWithPubKeySign();
  std::span<const unsigned char> pubKeyAesClient = cryptoC.getPublicKeyAes();
  // server
  CryptoSodium cryptoS(hashed, pubKeyAesClient, signatureWithPubKey);
  ASSERT_TRUE(cryptoS.isVerified());
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
  std::array<unsigned char, crypto_kx_PUBLICKEYBYTES> client_pk;
  std::array<unsigned char, crypto_kx_SECRETKEYBYTES> client_sk;
  std::array<unsigned char, crypto_kx_PUBLICKEYBYTES> server_pk;
  std::array<unsigned char, crypto_kx_SECRETKEYBYTES> server_sk;
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> client_rx;
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> client_tx;
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> server_rx;
  std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> server_tx;
  // Generate key pairs for client and server
  crypto_kx_keypair(client_pk.data(), client_sk.data());
  crypto_kx_keypair(server_pk.data(), server_sk.data());
  // Client-side key exchange
  ASSERT_TRUE(crypto_kx_client_session_keys(client_rx.data(), client_tx.data(), client_pk.data(), client_sk.data(), server_pk.data()) == 0);
  // Server-side key exchange
  ASSERT_TRUE(crypto_kx_server_session_keys(server_rx.data(), server_tx.data(), server_pk.data(), server_sk.data(), client_pk.data()) == 0);
  // Verify that the shared secrets match
  ASSERT_TRUE(client_rx == server_tx);
  ASSERT_TRUE(client_tx == server_rx);
  ASSERT_TRUE(client_tx != client_rx);
  ASSERT_TRUE(server_tx != server_rx);
  // in the application code:
  try {
    // client
    CryptoSodium cryptoC(utility::generateRawUUID());
    std::span<const unsigned char> hashed = cryptoC.getMsgHash();
    std::span<const unsigned char> signatureWithPubKey = cryptoC.getSignatureWithPubKeySign();
    std::span<const unsigned char> pubKeyAesClient = cryptoC.getPublicKeyAes();
    // server
    CryptoSodium cryptoS(hashed, pubKeyAesClient, signatureWithPubKey);
    std::span<const unsigned char> pubKeyAesServer = cryptoS.getPublicKeyAes();
    cryptoC.clientKeyExchange(pubKeyAesServer);
    std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES> clientKey = cryptoC.getAesKey();
    std::array<unsigned char, crypto_aead_aes256gcm_KEYBYTES> serverKey = cryptoS.getAesKey();
    // must match
    ASSERT_EQ(clientKey, serverKey);
  }
  catch (...) {
    // should be no exceptions
    ASSERT_TRUE(false);
  }
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
  testCompressEncrypt(CRYPTO::ENCRYPT, COMPRESSORS::LZ4);
}

TEST_F(CompressEncryptSodiumTest, ENCRYPT_COMPRESSORS_SNAPPY) {
  testCompressEncrypt(CRYPTO::ENCRYPT, COMPRESSORS::SNAPPY);
}

TEST_F(CompressEncryptSodiumTest, ENCRYPT_COMPRESSORS_ZSTD) {
  testCompressEncrypt(CRYPTO::ENCRYPT, COMPRESSORS::ZSTD);
}

TEST_F(CompressEncryptSodiumTest, ENCRYPT_COMPRESSORS_NONE) {
  testCompressEncrypt(CRYPTO::ENCRYPT, COMPRESSORS::NONE);
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
