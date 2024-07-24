/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <filesystem>

#include "Crypto.h"
#include "Logger.h"
#include "PayloadTransform.h"
#include "ServerOptions.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=CryptoTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=PayloadTransformTest*; done

TEST(CryptoTest, 1) {
  CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
  Crypto::_rng.GenerateBlock(key, key.size());
  std::string_view data = TestEnvironment::_source;
  std::string_view cipher = Crypto::encrypt(key, data);
  std::string_view decrypted = Crypto::decrypt(key, cipher);
  ASSERT_EQ(data, decrypted);
}

struct PayloadTransformTest : testing::Test {
  void test(CRYPTO encrypted, COMPRESSORS compressor) {
    std::string_view data = TestEnvironment::_source;
    ServerOptions::_compressor = compressor;
    ServerOptions::_encrypted = encrypted;
    HEADER header{HEADERTYPE::SESSION, 0, data.size(), compressor, encrypted, DIAGNOSTICS::NONE, STATUS::NONE,0};
    CryptoPP::SecByteBlock key(CryptoPP::AES::MAX_KEYLENGTH);
    Crypto::_rng.GenerateBlock(key, key.size());
    std::string_view transformed = payloadtransform::compressEncrypt(key, data, header);
    std::string_view restored = payloadtransform::decryptDecompress(key, header, transformed);
    ASSERT_EQ(data, restored);
  }

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(PayloadTransformTest, ENCRYPTED_LZ4) {
  test(CRYPTO::ENCRYPTED, COMPRESSORS::LZ4);
}

TEST_F(PayloadTransformTest, ENCRYPTED_NONE) {
  test(CRYPTO::ENCRYPTED, COMPRESSORS::NONE);
}

TEST_F(PayloadTransformTest, NOTENCRYPTED_NONE) {
  test(CRYPTO::NONE, COMPRESSORS::NONE);
}

TEST_F(PayloadTransformTest, NOTENCRYPTED_LZ4) {
  test(CRYPTO::NONE, COMPRESSORS::LZ4);
}
