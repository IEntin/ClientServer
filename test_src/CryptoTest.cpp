/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <cmath>

#include <boost/asio.hpp>

#include "Crypto.h"
#include <cryptopp/rsa.h>

#include "TestEnvironment.h"
#include "Utility.h"

// for i in {1..10}; do ./testbin --gtest_filter=CryptoTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=CompressEncryptTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=AuthenticationTest*; done

TEST(CryptoTest, 1) {
  auto crypto(std::make_shared<Crypto>());
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  CryptoPP::AutoSeededRandomPool prng;
  prng.GenerateBlock(key, key.size());
  // must be a copy
  std::string data = TestEnvironment::_source;
  crypto->encrypt(TestEnvironment::_buffer, true, data);
  crypto->decrypt(TestEnvironment::_buffer, data);
  ASSERT_EQ(TestEnvironment::_source, data);
  // must be a copy
  data = TestEnvironment::_source;
  crypto->encrypt(TestEnvironment::_buffer, false, data);
  crypto->decrypt(TestEnvironment::_buffer, data);
  ASSERT_EQ(TestEnvironment::_source, data);
}

struct CompressEncryptTest : testing::Test {
  void testCompressEncrypt(bool encrypt, COMPRESSORS compressor) {
    auto crypto(std::make_shared<Crypto>());
    CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::AutoSeededRandomPool prng;
    prng.GenerateBlock(key, key.size());
    // must be a copy
    std::string data = TestEnvironment::_source;
    HEADER header{ HEADERTYPE::SESSION,
		   data.size(),
		   compressor,
		   DIAGNOSTICS::NONE,
		   STATUS::NONE,
		   0 };
    utility::compressEncrypt(TestEnvironment::_buffer, encrypt, header, crypto, data);
    HEADER restoredHeader;    
    utility::decryptDecompress(TestEnvironment::_buffer, restoredHeader, crypto, data);
    ASSERT_EQ(header, restoredHeader);
    ASSERT_EQ(data, TestEnvironment::_source);
  }
  void TearDown() {}
};

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_LZ4) {
  testCompressEncrypt(true, COMPRESSORS::LZ4);
}

TEST_F(CompressEncryptTest, ENCRYPT_COMPRESSORS_NONE) {
  testCompressEncrypt(true, COMPRESSORS::NONE);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_LZ4) {
  testCompressEncrypt(false, COMPRESSORS::LZ4);
}

TEST_F(CompressEncryptTest, NOTENCRYPT_COMPRESSORS_NONE) {
  testCompressEncrypt(false, COMPRESSORS::NONE);
}

TEST(AuthenticationTest, 1) {
  // Generate RSA key pair
  CryptoPP::AutoSeededRandomPool rng;
  CryptoPP::RSA::PrivateKey privateKey;
  privateKey.GenerateRandomWithKeySize(rng, rsaKeySize);
  CryptoPP::RSA::PublicKey publicKey;
  publicKey.AssignFrom(privateKey);
  // Message to sign
  std::string message("authenticating...");
  // Sign the message
  CryptoPP::RSASSA_PKCS1v15_SHA256_Signer signer(privateKey);
  std::string signature;
  CryptoPP::StringSource ss(message,
    true, 
    new CryptoPP::SignerFilter(rng,
      signer,
      new CryptoPP::StringSink(signature)
    )
  );
  // Verify the signature
  CryptoPP::RSASSA_PKCS1v15_SHA256_Verifier verifier(publicKey);
  bool result = verifier.VerifyMessage(
    reinterpret_cast<const CryptoPP::byte*>(message.data()), message.length(),
    reinterpret_cast<const CryptoPP::byte*>(signature.data()), signature.length()
  );
  ASSERT_TRUE(result);
}

TEST(CryptoKeySerializationTest, 1) {
  auto crypto(std::make_shared<Crypto>());
  CryptoPP::RSA::PrivateKey privateKey;
  auto [success, serialized] = crypto->encodeRsaPublicKey(privateKey);
  ASSERT_TRUE(success);
  CryptoPP::RSA::PublicKey publicKey;
  ASSERT_TRUE(crypto->decodeRsaPublicKey(serialized, publicKey));
}
