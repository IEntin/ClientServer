/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"
#include "CommonConstants.h"
#include "Logger.h"
#include "TestEnvironment.h"
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>
#include <filesystem>

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
