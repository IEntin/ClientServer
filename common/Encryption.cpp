/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Encryption.h"
#include "CommonConstants.h"
#include "Logger.h"
#include "aes.h"
#include "modes.h"
#include "filters.h"
#include <filesystem>

std::vector<unsigned char> Encryption::_key(CryptoPP::AES::MAX_KEYLENGTH);
std::vector<unsigned char> Encryption::_iv(CryptoPP::AES::BLOCKSIZE);

void Encryption::initialize() {
  CryptoPP::byte key[CryptoPP::AES::MAX_KEYLENGTH];
  CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
  memcpy(_key.data(), key, CryptoPP::AES::MAX_KEYLENGTH);
  memcpy(_iv.data(), iv, CryptoPP::AES::BLOCKSIZE);
  try {
    std::ofstream keyFile(CRYPTO_KEY_FILE_NAME);
    std::filesystem::permissions(CRYPTO_KEY_FILE_NAME, std::filesystem::perms::owner_all);
    std::copy(_key.begin(), _key.end(), std::ostream_iterator<unsigned char>(keyFile));
    std::ofstream ivFile(CRYPTO_IV_FILE_NAME);
    std::filesystem::permissions(CRYPTO_IV_FILE_NAME, std::filesystem::perms::owner_all);
    std::copy(_iv.begin(), _iv.end(), std::ostream_iterator<unsigned char>(ivFile));
  }
  catch (const std::exception& e) {
    LogError << e.what();
  }
}

bool Encryption::recoverKeyAndIv(std::vector<unsigned char>& key,
				 std::vector<unsigned char>& iv) {
  try {
    std::ifstream keyFs(CRYPTO_KEY_FILE_NAME);
    std::copy(std::istream_iterator<unsigned char>(keyFs), 
	      std::istream_iterator<unsigned char>(), 
	      std::back_inserter(key));

    std::ifstream ivFs(CRYPTO_IV_FILE_NAME);
    std::copy(std::istream_iterator<unsigned char>(ivFs), 
	      std::istream_iterator<unsigned char>(), 
	      std::back_inserter(iv));
    return true;
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return false;
  }
}

bool Encryption::encrypt(std::string_view source,
			 const std::vector<unsigned char>& key,
			 const std::vector<unsigned char>& iv,
			 std::string& cipher) {
  try {
    CryptoPP::AES::Encryption aesEncryption(key.data(), CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv.data());

    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipher));
    stfEncryptor.Put(reinterpret_cast<const unsigned char*>(source.data()), source.size());
    stfEncryptor.MessageEnd();
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return false;
  }
  return true;
}

bool Encryption::decrypt(std::string_view cipher,
			 const std::vector<unsigned char>& key,
			 const std::vector<unsigned char>& iv,
			 std::string& decrypted) {
  try {
    CryptoPP::AES::Decryption aesDecryption(key.data(), CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());

    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decrypted));
    stfDecryptor.Put(reinterpret_cast<const unsigned char*>(cipher.data()), cipher.size());
    stfDecryptor.MessageEnd();
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return false;
  }
  return true;
}
