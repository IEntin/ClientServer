/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"
#include "CommonConstants.h"
#include "CommonUtils.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "TestEnvironment.h"
#include <filesystem>

// for i in {1..10}; do ./testbin --gtest_filter=CryptoTest*; done
// for i in {1..10}; do ./testbin --gtest_filter=CommonUtilsTest*; done

TEST(CryptoTest, 1) {
  // AES encryption uses a secret key of a variable length. This key is secretly
  // exchanged between two parties before communication begins.
  std::string in_out = TestEnvironment::_source;
  try {
    CryptoKeys cryptoKeys(TestEnvironment::_serverOptions);
    Crypto::encrypt(in_out, cryptoKeys);

    std::string decrypted;
    Crypto::decrypt(in_out, cryptoKeys, decrypted);

    ASSERT_EQ(TestEnvironment::_source.size(), decrypted.size());
    ASSERT_EQ(TestEnvironment::_source, decrypted);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
  std::filesystem::remove(CRYPTO_KEY_FILE_NAME);
  std::filesystem::remove(CRYPTO_IV_FILE_NAME);
}

struct CommonUtilsTest : testing::Test {
  void test(bool encrypted, COMPRESSORS compressor) {
    try {
      std::string in_out = TestEnvironment::_source;
      CryptoKeys cryptoKeys(TestEnvironment::_serverOptions);
      TestEnvironment::_serverOptions._encrypted = encrypted;
      TestEnvironment::_serverOptions._compressor = compressor;
      HEADER header;
      bool diagnostics = false;
      std::string_view compressEncrypt =
	commonutils::compressEncrypt(TestEnvironment::_serverOptions,
				     cryptoKeys,
				     in_out,
				     header,
				     diagnostics,
				     STATUS::NONE);
      std::string in = in_out;
      std::string_view result =
	commonutils::decryptDecompress(cryptoKeys, header, compressEncrypt);
      ASSERT_EQ(result.size(), TestEnvironment::_source.size());
      ASSERT_EQ(result, TestEnvironment::_source);
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
      ASSERT_TRUE(false);
    }
  }

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(CommonUtilsTest, ENCRYPTED_LZ4) {
  test(true, COMPRESSORS::LZ4);
}

TEST_F(CommonUtilsTest, ENCRYPTED_NONE) {
  test(true, COMPRESSORS::NONE);
}

TEST_F(CommonUtilsTest, NOTENCRYPTED_NONE) {
  test(false, COMPRESSORS::NONE);
}

TEST_F(CommonUtilsTest, NOTENCRYPTED_LZ4) {
  test(false, COMPRESSORS::LZ4);
}
