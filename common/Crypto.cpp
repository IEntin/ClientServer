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

CryptoPP::SecByteBlock CryptoKey::_key(ServerOptions::_cryptoKeySize);
bool CryptoKey::_valid = false;

void CryptoKey::showKey() {
  Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
  logger << "KEY SIZE: " << _key.size() << '\n' << "KEY: ";
  boost::algorithm::hex(_key, std::ostream_iterator<char> { logger.getStream(), "" });
  logger << '\n';
}

bool CryptoKey::recover() {
  std::string keyStrRecovered = utility::readFile(CRYPTO_KEY_FILE_NAME);
  _key = { reinterpret_cast<const unsigned char*>(keyStrRecovered.data()), keyStrRecovered.size() };
  if (_key.empty())
    throw std::runtime_error("empty key");
  _valid = true;
  return _valid;
}

bool CryptoKey::initialize(const ServerOptions& options) {
  if (!std::filesystem::exists(CRYPTO_KEY_FILE_NAME) || options._invalidateKey) {
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
    }
  }
  else
    _valid = recover();
  return _valid;
}


// class Crypto

void Crypto::encrypt(std::string& data) {
  const auto& key = CryptoKey::_key;
  CryptoPP::AutoSeededRandomPool prng;
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  prng.GenerateBlock(iv, iv.size());
  static bool showOnce [[maybe_unused]] = showIv(iv);
  CryptoPP::AES::Encryption aesEncryption(key.data(), key.size());
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
  const auto& key = CryptoKey::_key;
  CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
  auto beg = reinterpret_cast<const unsigned char*>(data.data()) + data.size() - iv.size();
  std::copy(beg, beg + iv.size(), iv.data());
  data.resize(data.size() - iv.size());
  static thread_local std::string decrypted;
  decrypted.clear();
  try {
    CryptoPP::AES::Decryption aesDecryption(key.data(), key.size());
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

bool Crypto::showIv(const CryptoPP::SecByteBlock& iv) {
  Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
  logger << "IV : ";
  boost::algorithm::hex(iv, std::ostream_iterator<char> { logger.getStream(), "" });
  logger << '\n';
  return true;
}
