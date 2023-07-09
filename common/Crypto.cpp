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

CryptoKey::CryptoKey(const ServerOptions& options) :
  _key(options._cryptoKeySize) {
  if (!std::filesystem::exists(CRYPTO_KEY_FILE_NAME) || options._invalidateKey)
    _valid = generate();
    else
      _valid = recover();
}

CryptoKey::CryptoKey() {
  _valid = recover();
}

bool CryptoKey::generate() {
  CryptoPP::AutoSeededRandomPool prng;
  prng.GenerateBlock(_key, _key.size());
  std::string keyStr(reinterpret_cast<const char*>(&_key[0]), _key.size());
  if (!utility::writeFile(CRYPTO_KEY_FILE_NAME, keyStr))
    return false;
  return _valid;
}

bool CryptoKey::recover() {
  std::string keyStrRecovered = utility::readFile(CRYPTO_KEY_FILE_NAME);
  if (keyStrRecovered.empty()) {
    return false;
  }
  _key = { reinterpret_cast<const unsigned char*>(&keyStrRecovered[0]), keyStrRecovered.size() };
  _valid = true;
  return _valid;
}

void CryptoKey::showKey() {
  std::clog << "KEY SIZE: " << _key.size() << std::endl;
  CryptoPP::HexEncoder encoder(new CryptoPP::FileSink(std::clog));
  std::clog << "KEY: ";
  CryptoPP::StringSource(_key, _key.size(), true, new CryptoPP::Redirector(encoder));
  std::clog << std::endl;
}

void Crypto::encrypt(std::string& data) {
  CryptoPP::AutoSeededRandomPool prng;
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  prng.GenerateBlock(iv, iv.size());
  std::string ivStr(reinterpret_cast<const char*>(&iv[0]), iv.size());
  static CryptoKey key;
  if (key._key.empty())
    throw std::runtime_error("empty key");
  CryptoPP::AES::Encryption aesEncryption(key._key.data(), key._key.size());
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv.data());
  static thread_local std::string cipher;
  cipher.clear();
  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipher));
  stfEncryptor.Put(reinterpret_cast<const unsigned char*>(data.data()), data.size());
  stfEncryptor.MessageEnd();
  data.swap(cipher);
  data.append(ivStr);
}

void Crypto::decrypt(std::string& data) {
  static CryptoKey key;
  std::string ivStr = data.substr(data.size() - CryptoPP::AES::BLOCKSIZE, CryptoPP::AES::BLOCKSIZE);
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  iv = { reinterpret_cast<const unsigned char*>(&ivStr[0]), ivStr.size() };
  data.erase(data.size() - CryptoPP::AES::BLOCKSIZE, CryptoPP::AES::BLOCKSIZE);
  static thread_local std::string decrypted;
  decrypted.clear();
  if (key._key.empty())
    throw std::runtime_error("empty key");
  try {
    CryptoPP::AES::Decryption aesDecryption(key._key.data(), key._key.size());
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decrypted));
    stfDecryptor.Put(reinterpret_cast<const unsigned char*>(data.data()), data.size());
    stfDecryptor.MessageEnd();
    data.swap(decrypted);
  }
  catch (const std::exception& e) {
    std::string error(e.what());
    error.append("\n\n\tMake sure crypto files on client site are current!\n");
    throw std::runtime_error(error);
  }
}
