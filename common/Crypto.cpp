/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"
#include "CommonConstants.h"
#include "Logger.h"
#include "ServerOptions.h"
#include "Utility.h"
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <filesystem>

CryptoKeys::CryptoKeys(const ServerOptions& options) :
  _key(options._cryptoKeySize),
  _iv(CryptoPP::AES::BLOCKSIZE) {
  if (!(std::filesystem::exists(CRYPTO_KEY_FILE_NAME) &&
	std::filesystem::exists(CRYPTO_KEY_FILE_NAME)) || options._invalidateKeys)
    _valid = generate();
    else
      _valid = recover();
}

CryptoKeys::CryptoKeys() {
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
  if (keyStrRecovered.empty()) {
    return false;
  }
  _key = { reinterpret_cast<const unsigned char*>(&keyStrRecovered[0]), keyStrRecovered.size() };
  std::string ivStrRecovered = utility::readFile(CRYPTO_IV_FILE_NAME);
  _iv = { reinterpret_cast<const unsigned char*>(&ivStrRecovered[0]), ivStrRecovered.size() };
  _valid = true;
  return _valid;
}

void CryptoKeys::showKeys() {
  std::clog << "KEY SIZE: " << _key.size() << std::endl;
  CryptoPP::HexEncoder encoder(new CryptoPP::FileSink(std::clog));
  std::clog << "KEY: ";
  CryptoPP::StringSource(_key, _key.size(), true, new CryptoPP::Redirector(encoder));
  std::clog << "\nIV: ";
  CryptoPP::StringSource(_iv, _iv.size(), true, new CryptoPP::Redirector(encoder));
  std::clog << std::endl;
}

void Crypto::encrypt(std::string_view source,
		     const CryptoKeys& keys,
		     std::string& cipher) {
  if (keys._key.empty() || keys._iv.empty())
    throw std::runtime_error("empty keys");
  CryptoPP::AES::Encryption aesEncryption(keys._key.data(), keys._key.size());
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, keys._iv.data());
  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipher));
  stfEncryptor.Put(reinterpret_cast<const unsigned char*>(source.data()), source.size());
  stfEncryptor.MessageEnd();
}

void Crypto::decrypt(std::string_view cipher,
		     const CryptoKeys& keys,
		     std::string& decrypted) {
  if (keys._key.empty() || keys._iv.empty())
    throw std::runtime_error("empty keys");
  try {
    CryptoPP::AES::Decryption aesDecryption(keys._key.data(), keys._key.size());
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, keys._iv.data());
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decrypted));
    stfDecryptor.Put(reinterpret_cast<const unsigned char*>(cipher.data()), cipher.size());
    stfDecryptor.MessageEnd();
  }
  catch (const std::exception& e) {
    std::string error(e.what());
    error.append("\n\n\tMake sure crypto files on client site are current!\n");
    throw std::runtime_error(error);
  }
}
