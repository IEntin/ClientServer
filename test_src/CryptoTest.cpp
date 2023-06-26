/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"
#include "Logger.h"
#include "TestEnvironment.h"
#include "aes.h"
#include "modes.h"
#include "filters.h"

TEST(CryptoTest, 1) {
  // AES encryption uses a secret key of a variable length. This key is secretly
  // exchanged between two parties before communication begins.
  try {
    CryptoKeys cryptoKeys(true);
    std::string_view sourceView(TestEnvironment::_source);

    std::string cipher;
    ASSERT_TRUE(Crypto::encrypt(sourceView, cryptoKeys, cipher));

    std::string decrypted;
    ASSERT_TRUE(Crypto::decrypt(cipher, cryptoKeys, decrypted));

    ASSERT_EQ(TestEnvironment::_source.size(), decrypted.size());
    ASSERT_EQ(TestEnvironment::_source, decrypted);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
}
