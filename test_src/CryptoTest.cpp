/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <filesystem>

#include <cryptopp/osrng.h>

#include "CommonConstants.h"
#include "Crypto.h"
#include "Logger.h"
#include "PayloadTransform.h"
#include "ServerOptions.h"
#include "TestEnvironment.h"

// for i in {1..10}; do ./testbin --gtest_filter=CryptoTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=PayloadTransformTest*; done

TEST(CryptoTest, 1) {
  CryptoPP::SecByteBlock key;
  key.resize(KEY_LENGTH);
  CryptoPP::AutoSeededRandomPool prng;
  prng.GenerateBlock(key, key.size());
  std::string_view data = TestEnvironment::_source;
  std::string_view cipher = Crypto::encrypt(key, data);
  std::string_view decrypted = Crypto::decrypt(key, cipher);
  ASSERT_EQ(TestEnvironment::_source.size(), decrypted.size());
  ASSERT_TRUE(std::memcmp(TestEnvironment::_source.data(), decrypted.data(), decrypted.size()) == 0);
}

struct PayloadTransformTest : testing::Test {
  void test(bool encrypted, COMPRESSORS compressor) {
    std::string data = TestEnvironment::_source;
    ServerOptions::_encrypted = encrypted;
    ServerOptions::_compressor = compressor;
    HEADER header{HEADERTYPE::SESSION, 0, 0, compressor, encrypted, false, STATUS::NONE};
    CryptoPP::SecByteBlock key;
    key.resize(KEY_LENGTH);
    CryptoPP::AutoSeededRandomPool prng;
    prng.GenerateBlock(key, key.size());
    std::string_view transformed = payloadtransform::compressEncrypt(key, data, header);
    std::string_view restored = payloadtransform::decryptDecompress(key, header, transformed);
    ASSERT_EQ(restored.size(), TestEnvironment::_source.size());
    ASSERT_EQ(restored, TestEnvironment::_source);
  }

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(PayloadTransformTest, ENCRYPTED_LZ4) {
  test(true, COMPRESSORS::LZ4);
}

TEST_F(PayloadTransformTest, ENCRYPTED_NONE) {
  test(true, COMPRESSORS::NONE);
}

TEST_F(PayloadTransformTest, NOTENCRYPTED_NONE) {
  test(false, COMPRESSORS::NONE);
}

TEST_F(PayloadTransformTest, NOTENCRYPTED_LZ4) {
  test(false, COMPRESSORS::LZ4);
}
