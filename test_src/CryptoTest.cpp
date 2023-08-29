/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CommonConstants.h"
#include "PayloadTransform.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "TestEnvironment.h"
#include <filesystem>

// for i in {1..10}; do ./testbin --gtest_filter=CryptoTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=PayloadTransformTest*; done

TEST(CryptoTest, 1) {
  // AES encryption uses a secret key of a variable length. This key is secretly
  // exchanged between two parties before communication begins.
  std::string data = TestEnvironment::_source;
  try {
    CryptoKey::initialize(TestEnvironment::_serverOptions);
    
    Crypto::encrypt(data);

    Crypto::decrypt(data);

    ASSERT_EQ(TestEnvironment::_source.size(), data.size());
    ASSERT_EQ(TestEnvironment::_source, data);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
  std::filesystem::remove(CRYPTO_KEY_FILE_NAME);
}

struct PayloadTransformTest : testing::Test {
  void test(bool encrypted, COMPRESSORS compressor) {
    try {
      std::string data = TestEnvironment::_source;
      CryptoKey::initialize(TestEnvironment::_serverOptions);
      TestEnvironment::_serverOptions._encrypted = encrypted;
      TestEnvironment::_serverOptions._compressor = compressor;
      HEADER header;
      bool diagnostics = false;
      payloadtransform::compressEncrypt(TestEnvironment::_serverOptions,
					data,
					header,
					diagnostics,
					STATUS::NONE);
      payloadtransform::decryptDecompress(header, data);
      ASSERT_EQ(data.size(), TestEnvironment::_source.size());
      ASSERT_EQ(data, TestEnvironment::_source);
    }
    catch (const std::exception& e) {
      LogError << e.what() << '\n';
      ASSERT_TRUE(false);
    }
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
