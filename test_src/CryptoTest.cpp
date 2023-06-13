/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Encryption.h"
#include "Logger.h"
#include "TestEnvironment.h"
#include "aes.h"
#include "modes.h"
#include "filters.h"

TEST(CryptoTest, 1) {
  // AES encryption uses a secret key of a variable length. This key is secretly
  // exchanged between two parties before communication begins.
  try {
    CryptoPP::byte key[CryptoPP::AES::MAX_KEYLENGTH];
    CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];

    std::vector<unsigned char> savedKey(CryptoPP::AES::MAX_KEYLENGTH);
    std::memcpy(savedKey.data(), key, CryptoPP::AES::MAX_KEYLENGTH);
    std::vector<unsigned char> savedIv(CryptoPP::AES::BLOCKSIZE);
    std::memcpy(savedIv.data(), iv, CryptoPP::AES::BLOCKSIZE);

    std::string_view sourceView(TestEnvironment::_source);

    std::string cipher;
    ASSERT_TRUE(Encryption::encrypt(sourceView, savedKey, savedIv, cipher));

    std::string decrypted;
    ASSERT_TRUE(Encryption::decrypt(cipher, savedKey, savedIv, decrypted));

    ASSERT_EQ(TestEnvironment::_source.size(), decrypted.size());
    ASSERT_EQ(TestEnvironment::_source, decrypted);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
}
