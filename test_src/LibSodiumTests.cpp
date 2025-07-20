
/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <boost/algorithm/hex.hpp>

#include <sodium.h>

#include "ClientOptions.h"
#include "CryptoSodium.h"
#include "DebugLog.h"
#include "Header.h"
#include "TestEnvironment.h"
#include "Utility.h"

// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.authentication; done
// for i in {1..10}; do ./testbin --gtest_filter=CompressEncryptSodiumTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=LibSodiumTest.DHkeyExchange; done

TEST(LibSodiumTest, authentication) {
  DebugLog::setTitle("LibSodiumTest, authentication");
  // client
  CryptoSodium cryptoC(utility::generateRawUUID());
  const auto& hashed = cryptoC.getMsgHash();
  const auto& signatureWithPubKey = cryptoC.getSignatureWithPubKeySign();
  const auto&  pubKeyAesClient = cryptoC.getPublicKeyAes();
  // server
  CryptoSodium cryptoS(hashed, pubKeyAesClient, signatureWithPubKey);
  ASSERT_TRUE(cryptoS.isVerified());
}

TEST(LibSodiumTest, hashing) {
  CryptoSodium crypto(utility::generateRawUUID());
  const auto& hashed(crypto.getMsgHash());
  ASSERT_EQ(hashed.size(), crypto_generichash_BYTES);
  std::clog << "generichash:0x";
  boost::algorithm::hex(hashed, std::ostream_iterator<char> { std::clog });
  std::clog << '\n';
}

TEST(LibSodiumTest, DHkeyExchange) {
  DebugLog::setTitle("LibSodiumTest, DHkeyExchange");
  try {
    // client
    CryptoSodium cryptoC(utility::generateRawUUID());
    const auto& hashed = cryptoC.getMsgHash();
    const auto& pubKeyAesClient = cryptoC.getPublicKeyAes();
    const auto& signatureWithPubKey = cryptoC.getSignatureWithPubKeySign();
    // server
    CryptoSodium cryptoS(hashed, pubKeyAesClient, signatureWithPubKey);
    ASSERT_TRUE(cryptoS.isVerified());
    const auto& pubKeyAesServer = cryptoS.getPublicKeyAes();
    cryptoC.clientKeyExchange(pubKeyAesServer);
    // test encrypt - decrypt
    HEADER header{ HEADERTYPE::SESSION, 0, HEADER_SIZE + TestEnvironment::_source.size(), ClientOptions::_encryption,
		   COMPRESSORS::NONE, DIAGNOSTICS::NONE, STATUS::NONE, 0 };
    std::string_view encrypted = cryptoC.encrypt(TestEnvironment::_buffer,
						 header,
						 TestEnvironment::_source);
    ASSERT_TRUE(utility::isEncrypted(encrypted));
    std::string data(encrypted);
    cryptoS.decrypt(TestEnvironment::_buffer, data);
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
