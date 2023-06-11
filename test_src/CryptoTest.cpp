/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CommonConstants.h"
#include "Logger.h"
#include "TestEnvironment.h"
#include "aes.h"
#include "filters.h"
#include "modes.h"
#include <fstream>

TEST(CryptoTest, 1) {
  // AES encryption uses a secret key of a variable length. This key is secretly
  // exchanged between two parties before communication begins.
  try {
    std::vector<unsigned char> key;
    std::ifstream keyFs(CRYPTO_KEY_FILE_NAME);
    std::copy(std::istream_iterator<unsigned char>(keyFs), 
	      std::istream_iterator<unsigned char>(), 
	      std::back_inserter(key));

    std::vector<unsigned char> iv;
    std::ifstream ivFs(CRYPTO_IV_FILE_NAME);
    std::copy(std::istream_iterator<unsigned char>(ivFs), 
	      std::istream_iterator<unsigned char>(), 
	      std::back_inserter(iv));

    std::string ciphertext;
    std::string decryptedtext;

    // Create Cipher Text
    CryptoPP::AES::Encryption aesEncryption(key.data(), CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv.data());

    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(ciphertext));
    stfEncryptor.Put(reinterpret_cast<const unsigned char*>(TestEnvironment::_source.data()), TestEnvironment::_source.size());
    stfEncryptor.MessageEnd();

    // Decrypt
    CryptoPP::AES::Decryption aesDecryption(key.data(), CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());

    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
    stfDecryptor.Put(reinterpret_cast<const unsigned char*>(ciphertext.c_str()), ciphertext.size());
    stfDecryptor.MessageEnd();

    ASSERT_EQ(TestEnvironment::_source.size(), decryptedtext.size());
    ASSERT_EQ(TestEnvironment::_source, decryptedtext);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
  }
}
