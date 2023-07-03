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

struct CommonUtilsTestEncryptionFirst : testing::Test {
  void test(bool encrypted, COMPRESSORS compressor) {
    try {
      CryptoKeys cryptoKeys(true);
      TestEnvironment::_serverOptions._encrypted = encrypted;
      TestEnvironment::_serverOptions._compressor = compressor;
      HEADER header;
      std::string_view body;
      bool diagnostics = false;
      STATUS result =
	commonutils::compressEncrypt(TestEnvironment::_serverOptions,
				     cryptoKeys,
				     TestEnvironment::_source,
				     header,
				     body,
				     diagnostics,
				     STATUS::NONE);
      ASSERT_EQ(result, STATUS::NONE);
      ASSERT_EQ(extractPayloadSize(header), body.size());
      std::string_view decryptedDecompressed =
	commonutils::decryptDecompress(cryptoKeys, header, body);
      ASSERT_EQ(decryptedDecompressed, TestEnvironment::_source);
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
