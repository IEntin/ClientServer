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

TEST(CryptoTest, 1) {
  auto crypto(std::make_shared<Crypto>());
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  CryptoPP::AutoSeededRandomPool prng;
  prng.GenerateBlock(key, key.size());
  std::string_view data = TestEnvironment::_source;
  std::string_view cipher = crypto->encrypt(true, data);
  std::string_view decrypted = crypto->decrypt(cipher);
  ASSERT_EQ(data, decrypted);
  std::string_view cipher2 = crypto->encrypt(false, data);
  std::string_view decrypted2 = crypto->decrypt(cipher2);
  ASSERT_EQ(data, decrypted2);
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
    std::string_view transformed[[maybe_unused]] = utility::compressEncrypt(encrypt, header, crypto, data);
    HEADER restoredHeader;    
    std::string_view restored = utility::decryptDecompress(restoredHeader, crypto, data);
    ASSERT_EQ(header, restoredHeader);
    ASSERT_EQ(restored, TestEnvironment::_source);
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
  privateKey.GenerateRandomWithKeySize(rng, 2048);
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
