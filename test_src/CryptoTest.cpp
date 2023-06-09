/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "modes.h"
#include "aes.h"
#include "filters.h"
#include "TestEnvironment.h"

TEST(CryptoTest, 1) {
  //Key and IV setup
  //AES encryption uses a secret key of a variable length (128-bit, 196-bit or 256-   
  //bit). This key is secretly exchanged between two parties before communication   
  //begins. DEFAULT_KEYLENGTH= 16 bytes
  CryptoPP::byte key[ CryptoPP::AES::DEFAULT_KEYLENGTH ], iv[ CryptoPP::AES::BLOCKSIZE ];
  memset(key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH);
  memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);

  std::string ciphertext;
  std::string decryptedtext;

  // Create Cipher Text
  CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);

  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext));
  stfEncryptor.Put(reinterpret_cast<const unsigned char*>(TestEnvironment::_source.data()), TestEnvironment::_source.size());
  stfEncryptor.MessageEnd();

  // Decrypt
  CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

  CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
  stfDecryptor.Put(reinterpret_cast<const unsigned char*>(ciphertext.c_str()), ciphertext.size());
  stfDecryptor.MessageEnd();

  ASSERT_EQ(TestEnvironment::_source.size(), decryptedtext.size());
  ASSERT_EQ(TestEnvironment::_source, decryptedtext);
}
