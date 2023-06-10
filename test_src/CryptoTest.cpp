/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "modes.h"
#include "aes.h"
#include "filters.h"
#include "TestEnvironment.h"

TEST(CryptoTest, 1) {
  // AES encryption uses a secret key of a variable length. This key is secretly
  // exchanged between two parties before communication begins.

  CryptoPP::byte key1[CryptoPP::AES::MAX_KEYLENGTH];
  CryptoPP::byte iv1[CryptoPP::AES::BLOCKSIZE];

  std::string skey;
  skey.reserve(CryptoPP::AES::MAX_KEYLENGTH);
  memcpy(skey.data(), key1, CryptoPP::AES::MAX_KEYLENGTH);

  std::string siv;
  siv.reserve(CryptoPP::AES::BLOCKSIZE);
  memcpy(siv.data(), iv1, CryptoPP::AES::BLOCKSIZE);

  std::string ciphertext;
  std::string decryptedtext;

  // Create Cipher Text
  CryptoPP::AES::Encryption aesEncryption(key1, CryptoPP::AES::MAX_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv1);

  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(ciphertext));
  stfEncryptor.Put(reinterpret_cast<const unsigned char*>(TestEnvironment::_source.data()), TestEnvironment::_source.size());
  stfEncryptor.MessageEnd();

  memset(key1, 0x00, sizeof(key1));
  memset(iv1, 0x00, sizeof(iv1));

  CryptoPP::byte key2[CryptoPP::AES::MAX_KEYLENGTH];
  memcpy(key2, skey.c_str(), CryptoPP::AES::MAX_KEYLENGTH);
  CryptoPP::byte iv2[CryptoPP::AES::BLOCKSIZE];
  memcpy(iv2, siv.c_str(), CryptoPP::AES::BLOCKSIZE);

  // Decrypt
  CryptoPP::AES::Decryption aesDecryption(key2, CryptoPP::AES::MAX_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv2);

  CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
  stfDecryptor.Put(reinterpret_cast<const unsigned char*>(ciphertext.c_str()), ciphertext.size());
  stfDecryptor.MessageEnd();

  ASSERT_EQ(TestEnvironment::_source.size(), decryptedtext.size());
  ASSERT_EQ(TestEnvironment::_source, decryptedtext);
}
