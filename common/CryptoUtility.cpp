/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CryptoUtility.h"
#include "CommonConstants.h"
#include "Logger.h"
#include "Utility.h"
#include "osrng.h"

CryptoKeys::CryptoKeys(bool bmaster) :
  _key(CryptoPP::AES::MAX_KEYLENGTH), _iv(CryptoPP::AES::BLOCKSIZE) {
  if (bmaster)
    _valid = generate();
  else
    _valid = recover();
}

bool CryptoKeys::generate() {
  CryptoPP::AutoSeededRandomPool prng;
  prng.GenerateBlock(_key, _key.size());
  std::string keyStr(reinterpret_cast<const char*>(&_key[0]), _key.size());
  if (!utility::writeFile(CRYPTO_KEY_FILE_NAME, keyStr))
    return false;
  prng.GenerateBlock(_iv, _iv.size());
   std::string ivStr(reinterpret_cast<const char*>(&_iv[0]), _iv.size());
  if (!utility::writeFile(CRYPTO_IV_FILE_NAME, ivStr))
    return false;
  _valid = true;
  return _valid;
}

bool CryptoKeys::recover() {
  std::string keyStrRecovered = utility::readFile(CRYPTO_KEY_FILE_NAME);
  _key = { reinterpret_cast<const unsigned char*>(&keyStrRecovered[0]), keyStrRecovered.size() };
  std::string ivStrRecovered = utility::readFile(CRYPTO_IV_FILE_NAME);
  _iv = { reinterpret_cast<const unsigned char*>(&ivStrRecovered[0]), ivStrRecovered.size() };
  _valid = true;
  return true;
}

bool CryptoUtility::encrypt(std::string_view source,
			    const CryptoKeys& keys,
			    std::string& cipher) {
  if (keys._key.empty() || keys._iv.empty())
    return false;
  try {
    CryptoPP::AES::Encryption aesEncryption(keys._key.data(), CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, keys._iv.data());
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

bool CryptoUtility::decrypt(std::string_view cipher,
			    const CryptoKeys& keys,
			    std::string& decrypted) {
  if (keys._key.empty() || keys._iv.empty())
    return false;
  try {
    CryptoPP::AES::Decryption aesDecryption(keys._key.data(), CryptoPP::AES::MAX_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, keys._iv.data());
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decrypted));
    stfDecryptor.Put(reinterpret_cast<const unsigned char*>(cipher.data()), cipher.size());
    stfDecryptor.MessageEnd();
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    LogError << "Decription problem.Crypto keys are not refreshed." << std::endl
	     << "Start the server and copy crypto files to clients!" << std::endl;
    std::exit(1);
    return false;
  }
  return true;
}
