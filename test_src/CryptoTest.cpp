/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Encryption.h"
#include "Logger.h"
#include "TestEnvironment.h"

TEST(CryptoTest, SERVER_ENCRYPTS) {
  // AES encryption uses a secret key of a variable length. This key is secretly
  // exchanged between two parties before communication begins.
  try {

    const std::vector<unsigned char>& keys = Encryption::getKey();
    const std::vector<unsigned char>& ivs = Encryption::getIv();

    std::string ciphertext;
    ASSERT_TRUE(Encryption::encrypt(TestEnvironment::_source, keys, ivs, ciphertext));

    std::vector<unsigned char> keyc;
    std::vector<unsigned char> ivc;

    ASSERT_TRUE(Encryption::recoverKeyAndIv(keyc, ivc));

    std::string decryptedtext;
    ASSERT_TRUE(Encryption::decrypt(ciphertext, keyc, ivc, decryptedtext));

    ASSERT_EQ(TestEnvironment::_source.size(), decryptedtext.size());
    ASSERT_EQ(TestEnvironment::_source, decryptedtext);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
}

TEST(CryptoTest, CLIENT_ENCRYPTS) {
  // AES encryption uses a secret key of a variable length. This key is secretly
  // exchanged between two parties before communication begins.
  try {
    std::vector<unsigned char> keyc;
    std::vector<unsigned char> ivc;

    ASSERT_TRUE(Encryption::recoverKeyAndIv(keyc, ivc));

    std::string ciphertext;
    ASSERT_TRUE(Encryption::encrypt(TestEnvironment::_source, keyc, ivc, ciphertext));

    const std::vector<unsigned char>& keys = Encryption::getKey();
    const std::vector<unsigned char>& ivs = Encryption::getIv();

    std::string decryptedtext;
    ASSERT_TRUE(Encryption::decrypt(ciphertext, keys, ivs, decryptedtext));

    ASSERT_EQ(TestEnvironment::_source.size(), decryptedtext.size());
    ASSERT_EQ(TestEnvironment::_source, decryptedtext);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
}
