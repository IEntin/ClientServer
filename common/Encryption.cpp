/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Encryption.h"
#include "CommonConstants.h"
#include "Logger.h"
#include "aes.h"
#include "modes.h"
#include "filters.h"

std::vector<unsigned char> Encryption::_key(CryptoPP::AES::MAX_KEYLENGTH);
std::vector<unsigned char> Encryption::_iv(CryptoPP::AES::BLOCKSIZE);

void Encryption::initialize() {
  CryptoPP::byte key[CryptoPP::AES::MAX_KEYLENGTH];
  CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
  memcpy(_key.data(), key, CryptoPP::AES::MAX_KEYLENGTH);
  memcpy(_iv.data(), iv, CryptoPP::AES::BLOCKSIZE);
  try {
    std::ofstream keyFile(CRYPTO_KEY_FILE_NAME);
    std::copy(_key.begin(), _key.end(), std::ostream_iterator<unsigned char>(keyFile));
    std::ofstream ivFile(CRYPTO_IV_FILE_NAME);
    std::copy(_iv.begin(), _iv.end(), std::ostream_iterator<unsigned char>(ivFile));
  }
  catch (const std::exception& e) {
    LogError << e.what();
  }
}
