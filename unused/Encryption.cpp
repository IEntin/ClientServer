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

CryptoKeys::CryptoKeys(bool bmaster) :
  _key(CryptoPP::AES::MAX_KEYLENGTH), _iv(CryptoPP::AES::BLOCKSIZE) {
  if (bmaster)
    _valid = generate();
  else
    _valid = recover();
}

bool CryptoKeys::generate() {
  CryptoPP::byte key[CryptoPP::AES::MAX_KEYLENGTH];
  CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
  memcpy(_key.data(), key, CryptoPP::AES::MAX_KEYLENGTH);
  memcpy(_iv.data(), iv, CryptoPP::AES::BLOCKSIZE);
  try {
    std::ofstream keyFile(CRYPTO_KEY_FILE_NAME, std::ios::binary);
    std::filesystem::permissions(CRYPTO_KEY_FILE_NAME, std::filesystem::perms::owner_all);
    std::copy(_key.begin(), _key.end(), std::ostream_iterator<unsigned char>(keyFile));
    std::ofstream ivFile(CRYPTO_IV_FILE_NAME, std::ios::binary);
    std::filesystem::permissions(CRYPTO_IV_FILE_NAME, std::filesystem::perms::owner_all);
    std::copy(_iv.begin(), _iv.end(), std::ostream_iterator<unsigned char>(ivFile));
  }
  catch (const std::exception& e) {
    LogError << e.what();
    return false;
  }
  return true;
}

bool CryptoKeys::recover() {
  try {
    if (!(std::filesystem::exists(CRYPTO_KEY_FILE_NAME) &&
	  std::filesystem::exists(CRYPTO_IV_FILE_NAME))) {
      LogError << "security files not found" << std::endl;
      return false;
    }
    std::ifstream keyFs(CRYPTO_KEY_FILE_NAME);
    std::copy(std::istream_iterator<unsigned char>(keyFs), 
	      std::istream_iterator<unsigned char>(), 
	      std::back_inserter(_key));
    std::ifstream ivFs(CRYPTO_IV_FILE_NAME);
    std::copy(std::istream_iterator<unsigned char>(ivFs), 
	      std::istream_iterator<unsigned char>(), 
	      std::back_inserter(_iv));
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return false;
  }
  return true;
}

bool Encryption::encrypt(std::string_view source,
			 const CryptoKeys& cryptoKeys,
			 std::string& cipher) {
  if (cryptoKeys._key.empty() || cryptoKeys._iv.empty())
    return false;
   try {
    CryptoPP::AES::Encryption aesEncryption(cryptoKeys._key.data(), CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, cryptoKeys._iv.data());

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
			 const CryptoKeys& cryptoKeys,
			 std::string& decrypted) {
  if (cryptoKeys._key.empty() || cryptoKeys._iv.empty())
    return false;
  try {
    CryptoPP::AES::Decryption aesDecryption(cryptoKeys._key.data(), CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, cryptoKeys._iv.data());

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
