/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"
#include "CommonConstants.h"
#include "CommonUtils.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "TestEnvironment.h"
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>
#include <filesystem>

// for i in {1..10}; do ./testbin --gtest_filter=CommonUtilsTest*; done

TEST(CryptoTest, 1) {
  // AES encryption uses a secret key of a variable length. This key is secretly
  // exchanged between two parties before communication begins.
  try {
    CryptoKeys cryptoKeys(true);
    std::string_view sourceView(TestEnvironment::_source);

    std::string cipher;
    Crypto::encrypt(sourceView, cryptoKeys, cipher);

    std::string decrypted;
    Crypto::decrypt(cipher, cryptoKeys, decrypted);

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
      CryptoKeys cryptoKeys(true);
      TestEnvironment::_serverOptions._encrypted = encrypted;
      TestEnvironment::_serverOptions._compressor = compressor;
      std::vector<char> data(TestEnvironment::_source.cbegin(), TestEnvironment::_source.cend());
      HEADER header;
      std::vector<char> body;
      bool diagnostics = false;
      STATUS result =
	commonutils::encryptCompressData(TestEnvironment::_serverOptions,
					 cryptoKeys,
					 data,
					 header,
					 body,
					 diagnostics,
					 STATUS::NONE);
      ASSERT_EQ(result, STATUS::NONE);
      std::string_view decryptedDecompressed =
	commonutils::decompressDecrypt(cryptoKeys, header, body);
      ASSERT_EQ(decryptedDecompressed, TestEnvironment::_source);
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
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

struct CommonUtilsTestEncryptionFirst : testing::Test {
  void test(bool encrypted, COMPRESSORS compressor) {
    try {
      CryptoKeys cryptoKeys(true);
      TestEnvironment::_serverOptions._encrypted = encrypted;
      TestEnvironment::_serverOptions._compressor = compressor;
      std::vector<char> data(TestEnvironment::_source.cbegin(), TestEnvironment::_source.cend());
      HEADER header;
      std::vector<char> body;
      bool diagnostics = false;
      STATUS result =
	commonutils::compressEncryptData(TestEnvironment::_serverOptions,
					 cryptoKeys,
					 data,
					 header,
					 body,
					 diagnostics,
					 STATUS::NONE);
      ASSERT_EQ(result, STATUS::NONE);
      std::string_view decryptedDecompressed =
	commonutils::decryptDecompress(cryptoKeys, header, body);
      ASSERT_EQ(decryptedDecompressed, TestEnvironment::_source);
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
    }
  }

  void TearDown() {
    TestEnvironment::reset();
  }
};

TEST_F(CommonUtilsTestEncryptionFirst, ENCRYPTED_LZ4) {
  test(true, COMPRESSORS::LZ4);
}

TEST_F(CommonUtilsTestEncryptionFirst, ENCRYPTED_NONE) {
  test(true, COMPRESSORS::NONE);
}

TEST_F(CommonUtilsTestEncryptionFirst, NOTENCRYPTED_NONE) {
  test(false, COMPRESSORS::NONE);
}

TEST_F(CommonUtilsTestEncryptionFirst, NOTENCRYPTED_LZ4) {
  test(false, COMPRESSORS::LZ4);
}
