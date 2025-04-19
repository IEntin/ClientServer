/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoPlPl.h"

#include "TestEnvironment.h"
#include "Utility.h"

// for i in {1..10}; do ./testbin --gtest_filter=CryptoTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=CompressEncryptTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=AuthenticationTest*; done

TEST(CryptoTest, 1) {
  auto crypto(std::make_shared<CryptoPlPl>(utility::generateRawUUID()));
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  CryptoPP::AutoSeededRandomPool prng;
  prng.GenerateBlock(key, key.size());
  crypto->setTestAesKey(key);
  HEADER header{ HEADERTYPE::SESSION,
		 0,
		 TestEnvironment::_source.size(),
		 CRYPTO::CRYPTOPP,
		 COMPRESSORS::NONE,
		 DIAGNOSTICS::NONE,
		 STATUS::NONE,
		 0 };
  // must be a copy
  std::string data(TestEnvironment::_source);
  std::string_view dataView =
    crypto->encrypt(TestEnvironment::_buffer, header, data);
  ASSERT_TRUE(utility::isEncrypted(dataView));
  data = dataView;
  crypto->decrypt(TestEnvironment::_buffer, data);
  HEADER restoredHeader;
  deserialize(restoredHeader, data.data());
  ASSERT_EQ(header, restoredHeader);
  ASSERT_FALSE(utility::isEncrypted(data));
  data.erase(0, HEADER_SIZE);
  ASSERT_EQ(data, TestEnvironment::_source);
}

struct CompressEncryptTest : testing::Test {
  void testCompressEncrypt(bool doEncrypt, COMPRESSORS compressor) {
    auto crypto(std::make_shared<CryptoPlPl>(utility::generateRawUUID()));
    CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::AutoSeededRandomPool prng;
    prng.GenerateBlock(key, key.size());
    crypto->setTestAesKey(key);
    // must be a copy
    std::string data = TestEnvironment::_source;
    HEADER header{ HEADERTYPE::SESSION,
		   0,
		   data.size(),
		   doEncrypt ? CRYPTO::CRYPTOPP : CRYPTO::NONE,
		   compressor,
		   DIAGNOSTICS::NONE,
		   STATUS::NONE,
		   0 };
    printHeader(header, LOG_LEVEL::ALWAYS);
    std::string_view dataView =
      utility::compressEncrypt(TestEnvironment::_buffer, header, crypto, data);
    ASSERT_EQ(utility::isEncrypted(dataView), doEncrypt);
    HEADER restoredHeader;
    data = dataView;
    utility::decryptDecompress(TestEnvironment::_buffer, restoredHeader, crypto, data);
    ASSERT_EQ(header, restoredHeader);
    ASSERT_EQ(data, TestEnvironment::_source);
  }
  void TearDown() {}
};

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_LZ4) {
  testCompressEncrypt(true, COMPRESSORS::LZ4);
}

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_SNAPPY) {
  testCompressEncrypt(true, COMPRESSORS::SNAPPY);
}

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_ZSTD) {
  testCompressEncrypt(true, COMPRESSORS::ZSTD);
}

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_NONE) {
  testCompressEncrypt(true, COMPRESSORS::NONE);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_LZ4) {
  testCompressEncrypt(false, COMPRESSORS::LZ4);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_SNAPPY) {
  testCompressEncrypt(false, COMPRESSORS::SNAPPY);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_ZSTD) {
  testCompressEncrypt(false, COMPRESSORS::ZSTD);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_NONE) {
  testCompressEncrypt(false, COMPRESSORS::NONE);
}

TEST(AuthenticationTest, 1) {
  // Generate RSA key pair
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::RSA::PrivateKey privateKey;
  privateKey.GenerateRandomWithKeySize(rng, RSA_KEY_SIZE);
  CryptoPP::RSA::PublicKey publicKey;
  publicKey.AssignFrom(privateKey);
  // Message to sign
  std::string message("test message");
  // Sign the message
  CryptoPP::RSASSA_PKCS1v15_SHA256_Signer signer(privateKey);
  std::string signature;
  CryptoPP::StringSource ss(message, true, new CryptoPP::SignerFilter(
    rng, signer, new CryptoPP::StringSink(signature)));
  ASSERT_EQ(signature.size(), RSA_KEY_SIZE >> 3);
  // Transfer the key and the signature
  // send
  CryptoPlPl crypto((utility::generateRawUUID()));
  auto [success, serialized] = crypto.encodeRsaPublicKey(privateKey);
  ASSERT_TRUE(success);
  signature += serialized;
  // receive
  std::string receivedSignature(signature, 0, RSA_KEY_SIZE >> 3);
  std::string_view serializedRsaPublicKey(signature.cbegin() + (RSA_KEY_SIZE >> 3), signature.cend());
  CryptoPP::RSA::PublicKey receivedRsaPublicKey;
  ASSERT_TRUE(crypto.decodeRsaPublicKey(serializedRsaPublicKey, receivedRsaPublicKey));
  // Verify the signature
  CryptoPP::RSASSA_PKCS1v15_SHA256_Verifier verifier(receivedRsaPublicKey);
  bool result = verifier.VerifyMessage(
    reinterpret_cast<const CryptoPP::byte*>(message.data()), message.length(),
    reinterpret_cast<const CryptoPP::byte*>(receivedSignature.data()), receivedSignature.size());
  ASSERT_TRUE(result);
  receivedSignature.erase(receivedSignature.cend() -1);
  result = verifier.VerifyMessage(
    reinterpret_cast<const CryptoPP::byte*>(message.data()), message.length(),
    reinterpret_cast<const CryptoPP::byte*>(receivedSignature.data()), receivedSignature.size());
  ASSERT_FALSE(result);
}
