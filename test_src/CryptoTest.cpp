/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <botan/auto_rng.h>
#include <botan/ber_dec.h>
#include <botan/der_enc.h>
//#include <botan/hash.h>
#include <botan/hex.h>
#include <botan/pk_keys.h>
#include <botan/pubkey.h>
#include <botan/rsa.h>

#include "Crypto.h"
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
