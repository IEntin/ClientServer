/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Crypto.h"
#include "CommonConstants.h"
#include "ServerOptions.h"
#include "Utility.h"
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <boost/algorithm/hex.hpp>
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
  std::string keyStr(reinterpret_cast<const char*>(_key.data()), _key.size());
  std::ofstream ofs(CRYPTO_KEY_FILE_NAME, std::ios::binary);
  std::filesystem::permissions(
    CRYPTO_KEY_FILE_NAME,
    std::filesystem::perms::owner_all | std::filesystem::perms::group_read,
    std::filesystem::perm_options::replace);
  if (ofs) {
    ofs << keyStr;
    _valid = true;
    return true;
  }
  return _valid;
}

bool CryptoKey::recover() {
  std::string keyStrRecovered = utility::readFile(CRYPTO_KEY_FILE_NAME);
  _key = { reinterpret_cast<const unsigned char*>(keyStrRecovered.data()), keyStrRecovered.size() };
  if (_key.empty())
    throw std::runtime_error("empty key");
  _valid = true;
  return _valid;
}

void CryptoKey::showKey() const {
  Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
  logger.getStream() << "KEY SIZE: " << _key.size() << '\n' << "KEY: ";
  boost::algorithm::hex(_key, std::ostream_iterator<char> { logger.getStream(), "" });
  logger.getStream() << std::endl;
}

void Crypto::encrypt(std::string& data) {
  static CryptoKey key;
  CryptoPP::AutoSeededRandomPool prng;
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  prng.GenerateBlock(iv, iv.size());
  CryptoPP::AES::Encryption aesEncryption(key._key.data(), key._key.size());
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv.data());
  static thread_local std::string cipher;
  cipher.clear();
  CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipher));
  stfEncryptor.Put(reinterpret_cast<const unsigned char*>(data.data()), data.size());
  stfEncryptor.MessageEnd();
  data.swap(cipher);
  data.append(reinterpret_cast<const char*>(iv.data()), iv.size());
}

void Crypto::decrypt(std::string& data) {
  static CryptoKey key;
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  auto beg = reinterpret_cast<const unsigned char*>(data.data()) + data.size() - iv.size();
  std::copy(beg, beg + iv.size(), iv.data());
  data.resize(data.size() - iv.size());
  static thread_local std::string decrypted;
  decrypted.clear();
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
    error.append("\n\n\tMake sure crypto key file on client site is current!\n");
    throw std::runtime_error(error);
  }
}

void Crypto::showIv(const CryptoPP::SecByteBlock& iv) {
  Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
  logger.getStream() << "IV : ";
  boost::algorithm::hex(iv, std::ostream_iterator<char> { logger.getStream(), "" });
  logger.getStream() << std::endl;
}
