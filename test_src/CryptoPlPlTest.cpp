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
  crypto->setDummyAesKey();
  HEADER header{ HEADERTYPE::SESSION,
		 0,
		 TestEnvironment::_source.size(),
		 CRYPTO::ENCRYPT,
		 COMPRESSORS::NONE,
		 DIAGNOSTICS::NONE,
		 STATUS::NONE,
		 0 };
  // must be a copy
  std::string data(TestEnvironment::_source);
  std::string_view dataView =
    crypto->encrypt(TestEnvironment::_buffer, header, data);
  ASSERT_TRUE(utility::isEncrypted(data));
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
  void testCompressEncrypt(CRYPTO encrypt, COMPRESSORS compressor) {
    auto crypto(std::make_shared<CryptoPlPl>(utility::generateRawUUID()));
    crypto->setDummyAesKey();
    // must be a copy
    std::string data = TestEnvironment::_source;
    HEADER header{ HEADERTYPE::SESSION,
		   0,
		   data.size(),
		   encrypt,
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

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_LZ4) {
  testCompressEncrypt(CRYPTO::ENCRYPT, COMPRESSORS::LZ4);
}

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_SNAPPY) {
  testCompressEncrypt(CRYPTO::ENCRYPT, COMPRESSORS::SNAPPY);
}

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_ZSTD) {
  testCompressEncrypt(CRYPTO::ENCRYPT, COMPRESSORS::ZSTD);
}

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_NONE) {
  testCompressEncrypt(CRYPTO::ENCRYPT, COMPRESSORS::NONE);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_LZ4) {
  testCompressEncrypt(CRYPTO::NONE, COMPRESSORS::LZ4);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_SNAPPY) {
  testCompressEncrypt(CRYPTO::NONE, COMPRESSORS::SNAPPY);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_ZSTD) {
  testCompressEncrypt(CRYPTO::NONE, COMPRESSORS::ZSTD);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_NONE) {
  testCompressEncrypt(CRYPTO::NONE, COMPRESSORS::NONE);
}

TEST(AuthenticationTest, 1) {
  // Generate RSA key pair
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::RSA::PrivateKey privateKey;
  privateKey.GenerateRandomWithKeySize(rng, RSA_KEY_SIZE);
  CryptoPP::RSA::PublicKey publicKey;
  publicKey.AssignFrom(privateKey);
  // Message to sign
  std::u8string message(utility::generateRawUUID());
  // Sign the message
  CryptoPP::RSASSA_PKCS1v15_SHA256_Signer signer(privateKey);
  std::string signature;
  CryptoPP::StringSource ss( { static_cast<const char*>(static_cast<const void*>(message.data())), message.size() },
			     true, new CryptoPP::SignerFilter(
    rng, signer, new CryptoPP::StringSink(signature)));
  ASSERT_EQ(signature.size(), RSA_KEY_SIZE >> 3);
  // Transfer the key and the signature
  CryptoPlPl crypto((utility::generateRawUUID()));
  auto [success, serialized] = crypto.encodeRsaPublicKey(privateKey);
  ASSERT_TRUE(success);
  std::string allReceived;
  allReceived.swap(signature);
  allReceived += serialized;

  std::string receivedSignature(allReceived, 0, RSA_KEY_SIZE >> 3);
  std::string_view serializedRsaPublicKey(allReceived.cbegin() + (RSA_KEY_SIZE >> 3), allReceived.cend());
  CryptoPP::RSA::PublicKey receivedRsaPublicKey;
  ASSERT_TRUE(crypto.decodeRsaPublicKey(serializedRsaPublicKey, receivedRsaPublicKey));
  // Verify the signature
  CryptoPP::RSASSA_PKCS1v15_SHA256_Verifier verifier(receivedRsaPublicKey);
  bool result = verifier.VerifyMessage(
    static_cast<const CryptoPP::byte*>(static_cast<const void*>(message.data())), message.length(),
    static_cast<const CryptoPP::byte*>(static_cast<const void*>(receivedSignature.data())), receivedSignature.size());
  ASSERT_TRUE(result);
  receivedSignature.erase(receivedSignature.cend() -1);
  result = verifier.VerifyMessage(
    static_cast<const CryptoPP::byte*>(static_cast<const void*>(message.data())), message.length(),
    static_cast<const CryptoPP::byte*>(static_cast<const void*>(receivedSignature.data())), receivedSignature.size());
  ASSERT_FALSE(result);
}

TEST(Base64EncodingTest, 1) {
  auto crypto(std::make_shared<CryptoPlPl>(utility::generateRawUUID()));
  std::span<unsigned char> pubKeyAes = crypto->getPublicKeyAes();
  CryptoPP::SecByteBlock original(pubKeyAes.data(), pubKeyAes.size());
  std::string encoded = CryptoPlPl::binary2string(pubKeyAes);
  std::vector<unsigned char> vect = CryptoPlPl::string2binary(encoded);
  CryptoPP::SecByteBlock recovered(vect.data(), vect.size());
  ASSERT_EQ(original, recovered);
}
